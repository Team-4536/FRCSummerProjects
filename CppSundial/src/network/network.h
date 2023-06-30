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

struct net_Prop {
    net_PropType type;
    void* data;

    U64 hashKey;
    net_Prop* hashNext;
};





void net_init();
void net_update();
void net_cleanup();

bool net_getConnected();

net_Prop* net_getProp(str name);


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

// Returns pointer to new Sock struct in <arena> if successful.
// Errors: failedAddrInfo, failedCreation
net_Sock* _net_sockCreate(str ip, str port, BumpAlloc& arena, net_SockErr* outError) {
    int err;
    *outError = net_sockErr_none;

    addrinfo hints;
    ZeroMemory( &hints, sizeof(hints) );
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    addrinfo* resInfo;
    // CLEANUP: perma allocations here
    err = getaddrinfo(str_cstyle(ip, arena), str_cstyle(port, arena), &hints, &resInfo);
    if(err != 0) {
        *outError = net_sockErr_failedAddrInfo;
        return nullptr;
    }


    SOCKET s = socket(resInfo->ai_family, resInfo->ai_socktype, resInfo->ai_protocol);
    if(s == INVALID_SOCKET) {
        freeaddrinfo(resInfo); // windows is a terrible platform
        *outError = net_sockErr_failedCreation;
        return nullptr;
    }

    // https://stackoverflow.com/questions/17227092/how-to-make-send-non-blocking-in-winsock
    u_long mode = 1;  // 1 to enable non-blocking socket
    ioctlsocket(s, FIONBIO, &mode);

    net_Sock* sock = BUMP_PUSH_NEW(arena, net_Sock);
    *sock = net_Sock();
    sock->addrInfo = resInfo;
    sock->s = s;

    return sock;
}


// return value indicates if socket has finished connecting to server
// Errors: net_sockErr_failedConnection
bool _net_sockAttemptConnect(net_Sock* s, net_SockErr* outError) {
    *outError = net_sockErr_none;
    int err = connect(s->s, s->addrInfo->ai_addr, (int)s->addrInfo->ai_addrlen);

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

void _net_sockFree(net_Sock* s) {
    if(s->addrInfo) { freeaddrinfo(s->addrInfo); }
    if(s->s != INVALID_SOCKET) { closesocket(s->s); }
}








struct net_Globs {

    WSADATA wsaData;

    net_Sock* simSocket;
    bool connected = false;

    BumpAlloc resArena; // sockets, props, misc stuff
    net_Prop** hash = nullptr; // array of ptrs for hash access
};

static net_Globs globs = net_Globs();
bool net_getConnected() { return globs.connected; }






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
net_Prop* _net_hashGet(str string) {

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
    str_printf(STR("Type: %i\n"), valType);

    if(msgKind == net_msgKind_UPDATE) {
        net_Prop* prop = _net_hashGet(name);
        if(!prop) {
            prop = BUMP_PUSH_NEW(globs.resArena, net_Prop);
            _net_hashInsert(name, prop);
        }

        prop->type = (net_PropType)valType;
        if(prop->type == net_propType_F64) {
            printf("%f\n", *((double*)&buffer[2+nameLen+1]));
        }
    }
    // TODO: events
    else { ASSERT(false); }

}











// Asserts on failures
void net_init() {

    bump_allocate(globs.resArena, NET_RES_SIZE + (sizeof(net_Prop) * NET_MAX_PROP_COUNT));
    globs.hash = (net_Prop**)arr_allocate0(sizeof(net_Prop*), NET_HASH_COUNT);


    // Initialize Winsock
    int err = WSAStartup(MAKEWORD(2,2), &globs.wsaData);
    if (err != 0) {
        printf("WSAStartup failed: %d\n", err);
        ASSERT(false);
    }


    net_SockErr sockErr;
    globs.simSocket = _net_sockCreate(STR("127.0.0.1"), STR("7000"), globs.resArena, &sockErr);
    if(sockErr) {
        printf("Error creating socket! Code: %i\n", sockErr);
        ASSERT(false);
    }
}







void net_update() {

    net_SockErr err;

    if(!globs.connected) {
        bool connected = _net_sockAttemptConnect(globs.simSocket, &err);

        if(err != net_sockErr_none) { /* failed */ }
        else if(connected) { globs.connected = true; }
        return;
    }





    U8 recvBuffer[NET_RECV_SIZE] = { 0 };
    int recvSize = recv(globs.simSocket->s, (char*)recvBuffer, NET_RECV_SIZE, 0);

    if (recvSize == SOCKET_ERROR) {
        if(WSAGetLastError() != WSAEWOULDBLOCK) {
            printf("Error recieving message! WSA code: %i\n", WSAGetLastError());
            globs.connected = false;
            closesocket(globs.simSocket->s);
        }
    }
    // buffer received
    else if (recvSize > 0) {
        _net_processMessage(recvBuffer, recvSize); }
    // size is 0, indicating shutdown
    else {
        globs.connected = false;
        closesocket(globs.simSocket->s);
        // TODO: proper reconnection procedure
    }
}

void net_cleanup() {
    _net_sockFree(globs.simSocket);
    WSACleanup();
}



#endif