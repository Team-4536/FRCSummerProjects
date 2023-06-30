#pragma once
#include <stdio.h>
#include <winsock2.h>
#include <WS2tcpip.h>
#include <iphlpapi.h>

#include "base/str.h"
#include "base/typedefs.h"
#include "base/allocators.h"




void net_init();
void net_update();
void net_cleanup();


// #ifdef NET_IMPL


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

void sock_free(Sock* s) {
    if(s->addrInfo) { freeaddrinfo(s->addrInfo); }
    if(s->s != INVALID_SOCKET) { closesocket(s->s); }
}



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
    ioctlsocket(globs.socket.s, FIONBIO, &mode);

    return sock;
}



void sock_send(Sock* sock, const U8* buf, U32 len) {
    U8 err;
    err = send(sock->s, (const char*)buf, len, 0);
    if (err == SOCKET_ERROR) { sock->err = err_sock_errSending; }
}
















struct net_Globs {
    WSADATA wsaData;
    bool ok;
    BumpAlloc resArena;
    Sock socket;
};

static net_Globs globs = net_Globs();


void net_init() {

    // Initialize Winsock
    int err = WSAStartup(MAKEWORD(2,2), &globs.wsaData);
    if (err != 0) {
        printf("WSAStartup failed: %d\n", err);
        globs.ok = false;
    }

    bump_allocate(globs.resArena, 1000000);

    Sock* sock = sock_createAndConnect(STR("127.0.0.1"), STR("7000"), globs.resArena);
    if(sock->err == err_sock_failedConnection) { printf("Error connecting to server!\n"); }
    if(sock->err != err_sock_none) {
        printf("Error creating/connecting socket!\n");
        globs.ok = false;
    }
}

void net_update() {
    if(!globs.ok) { return; }

    str msg = STR("Hello python!");
    sock_send(&globs.socket, (const U8*)msg.chars, msg.length);
    if(globs.socket.err != err_sock_none) {
        printf("Error sending message!");
        globs.ok = false;
    }
}

void net_cleanup() {
    sock_free(&globs.socket);
    WSACleanup();
}



#endif