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
    net_propType_STR,
    net_propType_BOOL
};

enum net_MsgKind {
    net_msgKind_UPDATE,
    net_msgKind_EVENT
};

// NOTE: contains space to fit string chars
union net_PropData {
    S32 s32;
    F64 f64;
    U8 boo;

    struct {
        str str;
        U8 chars[256];
    };
};

struct net_Prop {
    str name;
    net_PropType type;
    net_PropData* data;

    U64 hashKey;
    net_Prop* hashNext;

    net_Prop* eventNext;
};



struct net_Message {
    net_MsgKind kind;
    str name;
    net_PropType dataType;
    net_PropData data;
    net_Message* next;
};





void net_init();
void net_update(BumpAlloc* scratch, float curTime);
void net_cleanup();

void net_resetTracked();

// is net connected to anything?
bool net_getConnected();
void net_getTracked(net_Prop*** outArr, U32* outCount);
net_Prop* net_getEvents();

net_Prop* net_hashGet(str string);

void net_putMessage(str name, S32 data, BumpAlloc* scratch);
void net_putMessage(str name, F64 data, BumpAlloc* scratch);
void net_putMessage(str name, bool data, BumpAlloc* scratch);
void net_putMessage(str name, str data, BumpAlloc* scratch);
void net_putEvent(str name, BumpAlloc* scratch);
void net_putMessage(net_Message message, BumpAlloc* scratch);


#ifdef NET_IMPL

#include <stdio.h>
#include <winsock2.h>
#include <WS2tcpip.h>
#include <iphlpapi.h>


#include "base/hashtable.h"
#include "base/arr.h"
#include "base/utils.h"


#define NET_HASH_COUNT 10000
#define NET_MAX_PROP_COUNT 10000
#define NET_RES_SIZE 10000
#define NET_PACKET_SIZE 1024
#define NET_RECV_BUFFER_SIZE 2048

#define NET_SEND_INTERVAL 1/50.0f
#define NET_SEND_BUFFER_SIZE 10000






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

    net_Prop* eventStart = nullptr;
    net_Prop* eventEnd = nullptr;


    U32 recvBufStartOffset = 0;
    U8* recvBuffer;

    float lastSentTime = 0;
    BumpAlloc sendArena;


    FILE* logFile;
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

    bump_allocate(&globs.sendArena, NET_SEND_BUFFER_SIZE);

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


// NOTE: Event props are put in the scratch arena passed when calling net_update()
net_Prop* net_getEvents() {
    return globs.eventStart;
}

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









// returns nullptr if failed, else ptr to data & increments current
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
info is always big endian
*/

constexpr bool _net_isSystemBigEndian()
{
    union {
        U32 i;
        char c[4];
    } x = {0x01020304};
    return x.c[0] == 1;
}


// NOTE: unsafe to use with overlapping src and dst
U8* _net_reverseBytes(U8* data, U32 size, U8* dest) {
    for(int i = 0; i < size; i++) {
        dest[i] = data[size - 1 - i]; }
    return dest;
}



void net_putMessage(str name, S32 data, BumpAlloc* scratch) {
    net_Message m;
    m.kind = net_msgKind_UPDATE;
    m.name = name;
    m.dataType = net_propType_S32;
    m.data.s32 = data;
    net_putMessage(m, scratch);
}
void net_putMessage(str name, bool data, BumpAlloc* scratch) {
    net_Message m;
    m.kind = net_msgKind_UPDATE;
    m.name = name;
    m.dataType = net_propType_BOOL;
    m.data.boo = data? 1 : 0;
    net_putMessage(m, scratch);
}
void net_putMessage(str name, F64 data, BumpAlloc* scratch) {
    net_Message m;
    m.kind = net_msgKind_UPDATE;
    m.name = name;
    m.dataType = net_propType_F64;
    m.data.f64 = data;
    net_putMessage(m, scratch);
}
void net_putMessage(str name, str data, BumpAlloc* scratch) {
    net_Message m;
    m.kind = net_msgKind_UPDATE;
    m.name = name;
    m.dataType = net_propType_F64;
    m.data.str = data;
    net_putMessage(m, scratch);
}
void net_putEvent(str name, BumpAlloc* scratch) {
    net_Message m;
    m.kind = net_msgKind_EVENT;
    m.name = name;
    m.dataType = net_propType_BOOL;
    m.data.boo = 0;
    net_putMessage(m, scratch);
}
void net_putMessage(net_Message message, BumpAlloc* scratch) {

    U32 bufSize = 4; // header
    bufSize += message.name.length;

    U32 dataLen = 0;
    if(message.dataType == net_propType_S32) { dataLen = sizeof(S32); }
    else if(message.dataType == net_propType_F64) { dataLen = sizeof(F64); }
    else if(message.dataType == net_propType_BOOL) { dataLen = sizeof(U8); }
    else if(message.dataType == net_propType_STR) { dataLen = message.data.str.length; }
    else { ASSERT(false); }
    ASSERT(dataLen <= 255);
    bufSize += dataLen;


    U8* buffer = BUMP_PUSH_ARR(&globs.sendArena, bufSize, U8);
    buffer[0] = message.kind;
    ASSERT(message.name.length <= 255);
    buffer[1] = message.name.length;
    buffer[2] = message.dataType;
    buffer[3] = dataLen;
    str_copy(message.name, &(buffer[4]));

    U8* dataLoc = &(buffer[4 + message.name.length]);
    if(message.dataType == net_propType_S32) { *(S32*)(dataLoc) = message.data.s32; }
    else if(message.dataType == net_propType_F64) { *(F64*)(dataLoc) = message.data.f64; }
    else if(message.dataType == net_propType_BOOL) { *dataLoc = message.data.boo; }
    else if(message.dataType == net_propType_STR) { str_copy(message.data.str, dataLoc); }
    else { ASSERT(false); }

    // NOTE: IF ADDING TYPES THAT HAVE DYNAMIC SIZING/IGNORE BYTE ORDER FROM SYSTEM CHANGE THIS
    // VVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVV
    if(message.dataType != net_propType_STR) {
        if(!_net_isSystemBigEndian()) {
            U8* n = _net_reverseBytes(dataLoc, dataLen, BUMP_PUSH_ARR(scratch, dataLen, U8));
            memcpy(dataLoc, n, dataLen);
        }
    }
}





// returns log for given message
StrList _net_processMessage(U8 kind, str name, U8* data, U8 dataType, U32 dataSize, BumpAlloc* scratch) {

    StrList log = StrList();

    if(kind != net_msgKind_UPDATE &&
       kind != net_msgKind_EVENT) {
        str_listAppend(&log, STR("[Invalid msg type]\t"), scratch);
        str_listAppend(&log, str_format(scratch, STR("%i\t"), (int)kind), scratch);
        return log;
    }


    if(dataType != net_propType_S32 &&
       dataType != net_propType_F64 &&
       dataType != net_propType_STR &&
       dataType != net_propType_BOOL) {
        str_listAppend(&log, str_format(scratch, STR("[Invalid data type]\t"), dataType), scratch);
        str_listAppend(&log, str_format(scratch, STR("%i\t"), (int)dataType), scratch);
        return log;
    }

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
            if(!_net_isSystemBigEndian()) {
                U8* buf = BUMP_PUSH_ARR(scratch, sizeof(F64), U8);
                prop->data->f64 = *((F64*)_net_reverseBytes(data, sizeof(F64), buf));
            }

            str_listAppend(&log, str_format(scratch, STR("%f\t"), prop->data->f64), scratch);
        }
        else if (prop->type == net_propType_S32) {
            prop->data->s32 = *((S32*)data);
            if(!_net_isSystemBigEndian()) {
                U8* buf = BUMP_PUSH_ARR(scratch, sizeof(S32), U8);
                prop->data->s32 = *((S32*)_net_reverseBytes(data, sizeof(S32), buf));
            }
            str_listAppend(&log, str_format(scratch, STR("%i\t"), prop->data->s32), scratch);
        }
        else if(prop->type == net_propType_STR) {
            prop->data->str = str_copy({ (const U8*)data, dataSize }, prop->data->chars);
            str_listAppend(&log, str_format(scratch, STR("%s\t"), prop->data->str), scratch);
        }
        else if(prop->type == net_propType_BOOL) {
            prop->data->boo = *data;
            str_listAppend(&log, str_format(scratch, STR("%b\t"), prop->data->boo), scratch);
        }
    }
    else if(kind == net_msgKind_EVENT) {
        net_Prop* p = BUMP_PUSH_NEW(scratch, net_Prop);

        p->name = str_copy(name, scratch);

        if(!globs.eventStart) {
            globs.eventStart = p;
            globs.eventEnd = p;
        }
        else {
            globs.eventEnd->eventNext = p;
            globs.eventEnd = p;
        }
    }

    return log;
}


// NOTE: messages are expected to be formatted correctly, if one isn't this will not be able to read future messages
// return value indicates how much of the buffer was processed
U32 _net_processPackets(U8* buf, U32 bufSize, BumpAlloc* scratch) {

    U8* cur = buf;
    while(true) {

        U8* cCopy = cur;
        U8* messageHeader = _net_getBytes(buf, bufSize, &cCopy, 4);
        if(!messageHeader) { break; }

        U8 msgKind = messageHeader[0];
        U8 nameLen = messageHeader[1];
        U8 valType = messageHeader[2];
        U8 valLen = messageHeader[3];

        U8* nameBuf = _net_getBytes(buf, bufSize, &cCopy, nameLen);
        if(!nameBuf) { break; }
        U8* dataBuf = _net_getBytes(buf, bufSize, &cCopy, valLen);
        if(!dataBuf) { break; }

        StrList log = _net_processMessage(msgKind, {nameBuf, nameLen}, dataBuf, valType, valLen, scratch);
        str_listAppend(&log, STR("\n"), scratch);
        str collected = str_listCollect(log, scratch);
        fwrite(collected.chars, 1, collected.length, globs.logFile);

        cur = cCopy;
    }

    return cur - buf;
}

// Event props are put in scratch, as well as misc things
// Tracked props are stored in a resArena global.
void net_update(BumpAlloc* scratch, float curTime) {

    globs.eventStart = nullptr;
    globs.eventEnd = nullptr;


    net_SockErr err;

    if(!globs.connected) {
        bool connected = _net_sockCreateConnect("localhost", "7000", &globs.simSocket, &err);

        if(err != net_sockErr_none) { // attempt reconnection forever on errors
            _net_sockCloseFree(&globs.simSocket); }
        else if(connected) {
            globs.connected = true;
            net_resetTracked();

            str s = STR("[CONNECTED]\n");
            fwrite(s.chars, 1, s.length, globs.logFile);
        }
        return;
    }



    // SEND LOOP ///////////////////////////////////////////////////////////////////////////

    /*
    if(curTime > globs.lastSentTime + NET_SEND_INTERVAL) {
        globs.lastSentTime = curTime;

        U8* buf = (U8*)globs.sendArena.start;
        while (buf < (U8*)globs.sendArena.end) {
            break;

            U32 overhead = (buf + NET_PACKET_SIZE) - (U8*)globs.sendArena.end;
            int res = send(globs.simSocket.s, (const char*)buf, NET_PACKET_SIZE - max(0, overhead), 0);
            if(res == SOCKET_ERROR) {
                if(WSAGetLastError() != WSAEWOULDBLOCK) {

                    // TODO: file write err handling
                    str s = str_format(scratch, STR("[SEND ERR]\t%i\n"), WSAGetLastError());
                    fwrite(s.chars, 1, s.length, globs.logFile);

                    globs.connected = false;
                    _net_sockCloseFree(&globs.simSocket);
                    break;
                }
            }
            else { buf += res; }
        }

        bump_clear(&globs.sendArena);
    }
    */

    // RECV LOOP ///////////////////////////////////////////////////////////////////////////

    while(true) {

        U8* buf = (globs.recvBuffer + globs.recvBufStartOffset);
        int recvSize = recv(globs.simSocket.s, (char*)buf, NET_PACKET_SIZE, 0);
        printf("recvd: %i\n", recvSize);

        if (recvSize == SOCKET_ERROR) {
            int err = WSAGetLastError();
            if(err != WSAEWOULDBLOCK) {

                str s = str_format(scratch, STR("[RECV ERR]\t%i\n"), err);
                fwrite(s.chars, 1, s.length, globs.logFile);

                globs.connected = false;
                _net_sockCloseFree(&globs.simSocket);
            }
            break;
        }
        // size is 0, indicating shutdown
        else if (recvSize == 0) {
            globs.connected = false;
            _net_sockCloseFree(&globs.simSocket);
            break;
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
            else { break; }
        }
    }


    if(!globs.connected) {
        str s = STR("[DISCONNECTED]\n");
        fwrite(s.chars, 1, s.length, globs.logFile);
    }
}

void net_cleanup() {

    str s = STR("[DISCONNECTED]\n");
    fwrite(s.chars, 1, s.length, globs.logFile);
    fclose(globs.logFile);

    _net_sockCloseFree(&globs.simSocket);

    WSACleanup();
}



#endif