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
void net_update();
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







struct net_Sock {
    SOCKET s = INVALID_SOCKET;
    addrinfo* addrInfo = nullptr; // NOTE: expected to be managed
};

// windows is a terrible platform
// return value indicates if socket has finished connecting to server
// Errors: net_sockErr_failedConnection, failedAddrInfo, failedCreation
bool _net_sockCreateConnect(const char* ip, const char* port, net_Sock* sock, net_SockErr* outError) {
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
        err = getaddrinfo(ip, port, &hints, &resInfo);
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
};

static net_Globs globs = net_Globs();

// Asserts on failures
void net_init() {

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







void _net_processMessage(U8* buffer, U32 size) {

    ASSERT(size >= 2);


    U8 msgKind = buffer[0];
    U8 nameLen = buffer[1];
    str name = { &buffer[2], nameLen };
    str_printf(STR("%s\n"), name);

    ASSERT(size >= 2 + nameLen);
    U8 valType = buffer[2 + nameLen];
    ASSERT(valType == net_propType_S32 || valType == net_propType_F64); // TODO: str
    void* val = &(buffer[2+nameLen+1]);

    if(msgKind == net_msgKind_UPDATE) {
        net_Prop* prop = net_hashGet(name);
        if(!prop) {
            prop = BUMP_PUSH_NEW(&globs.resArena, net_Prop);
            _net_hashInsert(name, prop);
            ARR_APPEND(globs.tracked, globs.trackedCount, prop);
            prop->name = str_copy(name, &globs.resArena);
            prop->data = BUMP_PUSH_NEW(&globs.resArena, net_PropData);
        }

        prop->type = (net_PropType)valType;
        if(prop->type == net_propType_F64) {
            prop->data->f64 = *((F64*)val);
        }
        else if (prop->type == net_propType_S32) {
            prop->data->s32 = *((S32*)val);
            // TODO: endianness
        }
    }
    // TODO: events
    else { ASSERT(false); }

}

void net_update() {

    net_SockErr err;

    if(!globs.connected) {
        bool connected = _net_sockCreateConnect("127.0.0.1", "7000", &globs.simSocket, &err);

        if(err != net_sockErr_none) { // attempt reconnection forever on errors
            _net_sockCloseFree(&globs.simSocket); }
        else if(connected) {
            globs.connected = true;
            net_resetTracked();
        }
        return;
    }



    U8 recvBuffer[NET_RECV_SIZE] = { 0 };
    int recvSize = recv(globs.simSocket.s, (char*)recvBuffer, NET_RECV_SIZE, 0);

    if (recvSize == SOCKET_ERROR) {
        if(WSAGetLastError() != WSAEWOULDBLOCK) {
            printf("Error recieving message! WSA code: %i\n", WSAGetLastError());
            globs.connected = false;
            _net_sockCloseFree(&globs.simSocket);
        }
    }
    // buffer received
    else if (recvSize > 0) {
        _net_processMessage(recvBuffer, recvSize); }
    // size is 0, indicating shutdown
    else {
        globs.connected = false;
        _net_sockCloseFree(&globs.simSocket);
    }
}

void net_cleanup() {
    _net_sockCloseFree(&globs.simSocket);
    WSACleanup();
}



#endif