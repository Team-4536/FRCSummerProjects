#pragma once

#include "base/str.h"
#include "base/typedefs.h"
#include "base/allocators.h"



enum net_SockErr {
    net_sockErr_none,
    net_sockErr_failedAddrInfo,
    net_sockErr_failedCreation,
    net_sockErr_failedConnection,
    net_sockErr_errSending,
};




enum net_PropType {
    net_propType_S32,
    net_propType_F64,
    net_propType_STR
};

enum net_MsgKind {
    net_msgKind_UPDATE,
    net_msgKind_EVENT
};

union net_PropData {
    S32 s32;
    F64 f64;
};

struct net_Prop {
    str name;
    net_PropType type;
    net_PropData* data;

    U64 hashKey;
    net_Prop* hashNext;
};





void net_init();
void net_update(BumpAlloc* scratch);
void net_cleanup();

void net_resetTracked();

// is net connected to anything?
bool net_getConnected();
void net_getTracked(net_Prop*** outArr, U32* outCount);

net_Prop* net_hashGet(str string);


#ifdef NET_IMPL

#include <stdio.h>
#include <winsock2.h>
#include <WS2tcpip.h>
#include <iphlpapi.h>


#include "base/hashtable.h"
#include "base/arr.h"


#define NET_HASH_COUNT 10000
#define NET_MAX_PROP_COUNT 10000
#define NET_RES_SIZE 10000
#define NET_RECV_SIZE 1024
#define NET_RECV_BUFFER_SIZE 2048







struct net_Sock {
    SOCKET s = INVALID_SOCKET;
    addrinfo* addrInfo = nullptr; // NOTE: expected to be managed
};

// windows is a terrible platform
// hostname can also be an IP adress
// return value indicates if socket has finished connecting to server
// Errors: net_sockErr_failedConnection, failedAddrInfo, failedCreation
bool _net_sockCreateConnect(const char* hostname, const char* port, net_Sock* sock, net_SockErr* outError) {
    *outError = net_sockErr_none;
    int err;

    // create new
    if(sock->s == INVALID_SOCKET) {
        addrinfo hints;
        ZeroMemory( &hints, sizeof(hints) );
        hints.ai_family = AF_INET;
        hints.ai_socktype = SOCK_STREAM;
        hints.ai_protocol = IPPROTO_TCP;

        addrinfo* resInfo;
        // CLEANUP: perma allocations here
        err = getaddrinfo(hostname, port, &hints, &resInfo);
        if(err != 0) {
            *outError = net_sockErr_failedAddrInfo;
            return false;
        }


        SOCKET s = socket(resInfo->ai_family, resInfo->ai_socktype, resInfo->ai_protocol);
        if(s == INVALID_SOCKET) {
            freeaddrinfo(resInfo); // windows is a terrible platform
            *outError = net_sockErr_failedCreation;
            return false;
        }

        // https://stackoverflow.com/questions/17227092/how-to-make-send-non-blocking-in-winsock
        u_long mode = 1;  // 1 to enable non-blocking socket
        ioctlsocket(s, FIONBIO, &mode);

        sock->addrInfo = resInfo;
        sock->s = s;
    }


    err = connect(sock->s, sock->addrInfo->ai_addr, (int)sock->addrInfo->ai_addrlen);

    if (err == SOCKET_ERROR) {
        err = WSAGetLastError();
        if(err == WSAEWOULDBLOCK) { return false; }
        else if(err == WSAEISCONN) { return true; }
        else {
            *outError = net_sockErr_failedConnection;
            return false;
        }
    }

    return false;
}

// windows is a terrible platform
void _net_sockCloseFree(net_Sock* s) {
    if(s->addrInfo) { freeaddrinfo(s->addrInfo); }
    s->addrInfo = nullptr;
    if(s->s != INVALID_SOCKET) { closesocket(s->s); }
    s->s = INVALID_SOCKET;
}





struct net_Globs {

    WSADATA wsaData;

    net_Sock simSocket = net_Sock();
    bool connected = false;

    BumpAlloc resArena; // props, names, data, tracking, hash
    net_Prop** hash = nullptr; // array of ptrs for hash access
    net_Prop** tracked = nullptr; // array of ptrs for list based access
    U32 trackedCount = 0;

    FILE* logFile;

    U32 recvBufStartOffset = 0;
    U8* recvBuffer;
};

static net_Globs globs = net_Globs();

// Asserts on failures
void net_init() {

    globs.recvBuffer = (U8*)arr_allocate0(1, NET_RECV_BUFFER_SIZE);

    bump_allocate(&globs.resArena,
        NET_RES_SIZE + // names / data
        (sizeof(net_Prop) * NET_MAX_PROP_COUNT) + // props
        (sizeof(net_Prop*) * NET_MAX_PROP_COUNT) + // tracked
        (sizeof(net_Prop*) * NET_HASH_COUNT) // hash
        );

    net_resetTracked();

    // Initialize Winsock
    int err = WSAStartup(MAKEWORD(2,2), &globs.wsaData);
    if (err != 0) {
        printf("WSAStartup failed: %d\n", err);
        ASSERT(false);
    }


    globs.logFile = fopen("sunLog.log", "w");
}

// clears and reallocs resArena
// resets tracked vec also
void net_resetTracked() {
    bump_clear(&globs.resArena);
    globs.hash = BUMP_PUSH_ARR(&globs.resArena, NET_HASH_COUNT, net_Prop*);
    globs.tracked = BUMP_PUSH_ARR(&globs.resArena, NET_MAX_PROP_COUNT, net_Prop*);
    globs.trackedCount = 0;
}

bool net_getConnected() { return globs.connected; }

//                  pointer to array of pointers
void net_getTracked(net_Prop*** outArr, U32* outCount) {
    *outArr = globs.tracked;
    *outCount = globs.trackedCount; }


void _net_hashInsert(str string, net_Prop* p) {

    U64 key = hash_hashStr(string);
    U64 idx = key % NET_HASH_COUNT;
    net_Prop* elem = globs.hash[idx];

    if(elem) {
        while(elem->hashNext) {
            if(elem->hashKey == key) { ASSERT(false); return; } // already exists
            elem = elem->hashNext;
        }
        elem->hashNext = p;
        p->hashKey = key;
    }
    else {
        globs.hash[idx] = p;
        p->hashKey = key;
    }
}

// returns nullptr if no element, else pointer to elem
net_Prop* net_hashGet(str string) {

    U64 key = hash_hashStr(string);
    net_Prop* elem = globs.hash[key % NET_HASH_COUNT];
    while(true) {
        if(elem == nullptr) { return nullptr; }
        if(elem->hashKey == key) { return elem; }
        elem = elem->hashNext;
    }
}







void _net_log(str s) {
    fwrite(s.chars, 1, s.length, globs.logFile);
}


// TODO: expand data types
// TODO: document/improve log spec
// TODO: events
// TODO: msg begin markers

// returns nullptr if failed, else ptr to data
U8* _net_getBytes(U8* buf, U32 bufSize, U8** current, U32 count) {

    ASSERT(*current >= buf);
    if((*current) + count > buf + bufSize) { return nullptr; }
    U8* data = (*current);
    *current += count;
    return data;
}


//CLEANUP: includes




/*
Message layout:
[U8 kind] [U8 length of name] [U8 data type] [U8 data size] [name] [data]
*/
// returns log for given message
StrList _net_processMessage(U8 kind, str name, void* data, U8 dataType, U32 dataSize, BumpAlloc* scratch) {

    StrList log = StrList();

    str_listAppend(&log, str_format(scratch, STR("%i\t"), (int)kind), scratch);
    if(kind != net_msgKind_UPDATE) { // TODO: events
        str_listAppend(&log, STR("Invalid msg type\t"), scratch);
        return log;
    }

    str_listAppend(&log, str_format(scratch, STR("%i\t"), (int)dataType), scratch);

    if(dataType != net_propType_S32 &&
       dataType != net_propType_F64) {
        str_listAppend(&log, str_format(scratch, STR("Invalid data type\t"), dataType), scratch);
        return log;
    }

    str_listAppend(&log, str_format(scratch, STR("%i\t"), (int)dataSize), scratch);
    str_listAppend(&log, str_format(scratch, STR("%s\t"), name), scratch);




    if(kind == net_msgKind_UPDATE) {
        net_Prop* prop = net_hashGet(name);

        // CONSTUCTION
        if(!prop) {
            prop = BUMP_PUSH_NEW(&globs.resArena, net_Prop);
            _net_hashInsert(name, prop);
            ARR_APPEND(globs.tracked, globs.trackedCount, prop);

            prop->name = str_copy(name, &globs.resArena);
            prop->data = BUMP_PUSH_NEW(&globs.resArena, net_PropData);
        }


        prop->type = (net_PropType)dataType;
        if(prop->type == net_propType_F64) {
            prop->data->f64 = *((F64*)data);

            str_listAppend(&log, str_format(scratch, STR("%f\t"), prop->data->f64), scratch);
        }
        else if (prop->type == net_propType_S32) {
            prop->data->s32 = *((S32*)data);
            // TODO: endianness
        }
    }

    return log;
}


// return value indicates how much of the buffer was processed
U32 _net_processPackets(U8* buf, U32 bufSize, BumpAlloc* scratch) {

    // _net_log(str_format(scratch, STR("[PACKET START] size: %i\n"), bufSize));

    U8* cur = buf;
    while(true) {

        U8* messageHeader = _net_getBytes(buf, bufSize, &cur, 4);
        if(!messageHeader) { break; }

        U8 msgKind = messageHeader[0];
        U8 nameLen = messageHeader[1];
        U8 valType = messageHeader[2];
        U8 valLen = messageHeader[3];

        // CLEANUP: giving messages a lil too much control
        U8* nameBuf = _net_getBytes(buf, bufSize, &cur, nameLen);
        if(!nameBuf) { break; }
        U8* dataBuf = _net_getBytes(buf, bufSize, &cur, valLen);
        if(!nameBuf) { break; }

        StrList log = _net_processMessage(msgKind, {nameBuf, nameLen}, dataBuf, valType, valLen, scratch);
        _net_log(str_listCollect(log, scratch));
        _net_log(STR("\n"));
    }

    // _net_log(str_format(scratch, STR("[PACKET END] rem: %i\n"), cur - buf));

    return cur - buf;
}




void net_update(BumpAlloc* scratch) {

    net_SockErr err;

    if(!globs.connected) {
        bool connected = _net_sockCreateConnect("localhost", "7000", &globs.simSocket, &err);

        if(err != net_sockErr_none) { // attempt reconnection forever on errors
            _net_sockCloseFree(&globs.simSocket); }
        else if(connected) {
            globs.connected = true;
            net_resetTracked();
            _net_log(str_format(scratch, STR("[CONNECTED]\n")));
        }
        return;
    }



    U8* buf = (globs.recvBuffer + globs.recvBufStartOffset);
    int recvSize = recv(globs.simSocket.s, (char*)buf, NET_RECV_SIZE, 0);

    if (recvSize == SOCKET_ERROR) {
        if(WSAGetLastError() != WSAEWOULDBLOCK) {
            _net_log(str_format(scratch, STR("[ERR]\t%i\n"), WSAGetLastError()));
            globs.connected = false;
            _net_sockCloseFree(&globs.simSocket);
        }
    }
    // size is 0, indicating shutdown
    else if (recvSize == 0) {
        globs.connected = false;
        _net_sockCloseFree(&globs.simSocket);
    }
    // buffer received
    else if (recvSize > 0) {

        U32 bufSize = globs.recvBufStartOffset + recvSize;
        U32 endOfMessages = _net_processPackets(globs.recvBuffer, bufSize, scratch);

        if(endOfMessages > 0) {
            U8* remainder = globs.recvBuffer + endOfMessages;
            U32 remSize = (bufSize - endOfMessages);
            memmove(globs.recvBuffer, remainder, remSize);
            globs.recvBufStartOffset = remSize;
        }
    }




    if(!globs.connected) {
        _net_log(str_format(scratch, STR("[DISCONNECTED]\n")));
    }
}

void net_cleanup() {

    _net_log(STR("[QUIT]"));
    fclose(globs.logFile);

    _net_sockCloseFree(&globs.simSocket);

    WSACleanup();
}



#endif