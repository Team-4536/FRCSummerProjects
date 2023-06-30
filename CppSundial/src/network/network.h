#pragma once
#include <stdio.h>
#include <winsock2.h>
#include <WS2tcpip.h>
#include <iphlpapi.h>

#include "base/str.h"
#include "base/typedefs.h"
#include "base/allocators.h"

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



enum err_sock {
    err_sock_none,
    err_sock_failedAddrInfo,
    err_sock_failedCreation,
    err_sock_failedConnection,
    err_sock_errSending,
};


struct Sock {
    SOCKET s = INVALID_SOCKET;
    addrinfo* addrInfo = nullptr; // NOTE: expected to be managed
    err_sock err = err_sock_none;
};




void net_init();
void net_update();
void net_cleanup();

bool net_getOk();

net_Prop* net_getProp(str name);


#ifdef NET_IMPL

#include "base/hashtable.h"
#include "base/arr.h"


#define NET_HASH_COUNT 10000
#define NET_MAX_PROP_COUNT 10000
#define NET_RES_SIZE 10000
#define NET_RECV_SIZE 1024

struct net_Globs {
    WSADATA wsaData;
    bool ok = true;
    Sock* socket;

    BumpAlloc resArena;
    net_Prop** hash = nullptr;
};

static net_Globs globs = net_Globs();


bool net_getOk() { return globs.ok; }




Sock* sock_createAndConnect(str ip, str port, BumpAlloc& arena) {
    U32 err;

    Sock* sock = BUMP_PUSH_NEW(arena, Sock);
    *sock = Sock();

    addrinfo hints;
    ZeroMemory( &hints, sizeof(hints) );
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    err = getaddrinfo(str_cstyle(ip, arena), str_cstyle(port, arena), &hints, &sock->addrInfo);
    if(err != 0) { sock->err = err_sock_failedAddrInfo; }

    if(!sock->err) {
        sock->s = socket(sock->addrInfo->ai_family, sock->addrInfo->ai_socktype, sock->addrInfo->ai_protocol);
        if(sock->s == INVALID_SOCKET) { sock->err = err_sock_failedCreation; }
    }

    if(!sock->err) {
        err = connect(sock->s, sock->addrInfo->ai_addr, (int)sock->addrInfo->ai_addrlen);
        if (err == SOCKET_ERROR) { sock->err = err_sock_failedConnection; }
    }

    // https://stackoverflow.com/questions/17227092/how-to-make-send-non-blocking-in-winsock
    u_long mode = 1;  // 1 to enable non-blocking socket
    ioctlsocket(sock->s, FIONBIO, &mode);

    return sock;
}




void net_init() {

    bump_allocate(globs.resArena, NET_RES_SIZE + (sizeof(net_Prop) * NET_MAX_PROP_COUNT));
    globs.hash = (net_Prop**)arr_allocate0(sizeof(net_Prop*), NET_HASH_COUNT);


    // Initialize Winsock
    int err = WSAStartup(MAKEWORD(2,2), &globs.wsaData);
    if (err != 0) {
        printf("WSAStartup failed: %d\n", err);
        globs.ok = false;
    }

    globs.socket = sock_createAndConnect(STR("127.0.0.1"), STR("7000"), globs.resArena);

    if(globs.socket->err == err_sock_failedConnection) { printf("Error connecting to server!\n"); }
    if(globs.socket->err != err_sock_none) {
        printf("Error creating/connecting socket!\n");
        globs.ok = false;
    }
}










void sock_free(Sock* s) {
    if(s->addrInfo) { freeaddrinfo(s->addrInfo); }
    if(s->s != INVALID_SOCKET) { closesocket(s->s); }
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
net_Prop* _net_hashGet(str string) {

    U64 key = hash_hashStr(string);
    net_Prop* elem = globs.hash[key % NET_HASH_COUNT];
    while(true) {
        if(elem == nullptr) { return nullptr; }
        if(elem->hashKey == key) { return elem; }
        elem = elem->hashNext;
    }
}





void _net_getMessage() {

    U8 recvBuffer[NET_RECV_SIZE] = { 0 };
    int recvSize = recv(globs.socket->s, (char*)recvBuffer, NET_RECV_SIZE, 0);

    if (recvSize == SOCKET_ERROR) {
       if(WSAGetLastError() == WSAEWOULDBLOCK) {
            // .. waiting
            // printf("waiting...\n");
        } else {
            printf("Error recieving message! %i\n", WSAGetLastError());
            globs.ok = false;
        }
        return;
    }
    else if (recvSize > 0) {
        ASSERT(recvSize >= 2);


        U8 msgKind = recvBuffer[0];
        U8 nameLen = recvBuffer[1];
        str name = { &recvBuffer[2], nameLen };
        str_printf(STR("%s\n"), name);

        ASSERT(recvSize >= 2 + nameLen);
        U8 valType = recvBuffer[2 + nameLen];
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
                printf("%f\n", *((double*)&recvBuffer[2+nameLen+1]));
            }
        }
        // TODO: events
        else { ASSERT(false); }
    }
    else {
        printf("Socket closed!");
        globs.ok = false;
    }
}





void net_update() {
    if(!globs.ok) { return; }
    _net_getMessage();
}

void net_cleanup() {
    sock_free(globs.socket);
    WSACleanup();
}



#endif