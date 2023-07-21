#pragma once

#include "base/str.h"
#include "base/typedefs.h"
#include "base/allocators.h"
#include "network.h"




void nets_init();
void nets_cleanup();


// is net connected to anything?
bool nets_getConnected();
// NOTE: keep this string allocated because it gets used a lot
void nets_setTargetIp(str s);


// For now, logs and replay stuff will only work on recieved data, not sent stuff

net_Frame* nets_update(BumpAlloc* scratch, BumpAlloc* res, net_Table* table, float curTime);
void nets_putMessage(net_Prop message, BumpAlloc* scratch);
void net_putMessage(str name, F64 data, BumpAlloc* scratch);

#ifdef NET_IMPL

#include <stdio.h>
#include <winsock2.h>
#include <WS2tcpip.h>
#include <iphlpapi.h>

#include "base/hashtable.h"
#include "base/arr.h"
#include "base/utils.h"

#define NET_PACKET_SIZE 1024
#define NET_RECV_BUFFER_SIZE 2048
#define NET_SEND_BUFFER_SIZE 2048

#define NET_SEND_INTERVAL (1/50.0f)
#define NET_PORT "7000"





enum nets_SockErr {
    net_sockErr_none,
    net_sockErr_failedAddrInfo,
    net_sockErr_failedCreation,
    net_sockErr_failedConnection,
    net_sockErr_errSending,
};

struct nets_Sock {
    SOCKET s = INVALID_SOCKET;
    addrinfo* addrInfo = nullptr; // NOTE: expected to be managed
};

// windows is a terrible platform
// hostname can also be an IP adress
// return value indicates if socket has finished connecting to server
// Errors: net_sockErr_failedConnection, failedAddrInfo, failedCreation
bool _nets_sockCreateConnect(const char* hostname, const char* port, nets_Sock* sock, nets_SockErr* outError) {
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
void _nets_sockCloseFree(nets_Sock* s) {
    if(s->addrInfo) { freeaddrinfo(s->addrInfo); }
    s->addrInfo = nullptr;
    if(s->s != INVALID_SOCKET) { closesocket(s->s); }
    s->s = INVALID_SOCKET;
}





struct nets_Globs {

    WSADATA wsaData;
    str targetIp = { nullptr, 0 };
    nets_Sock simSocket = nets_Sock();
    bool connected = false;

    U32 recvBufStartOffset = 0;
    U8* recvBuffer = nullptr;
    float lastSendTime = 0;
    BumpAlloc sendArena;
};

static nets_Globs globs = nets_Globs();

// Asserts on failures
void nets_init() {

    globs.recvBuffer = (U8*)arr_allocate0(1, NET_RECV_BUFFER_SIZE);
    bump_allocate(&globs.sendArena, NET_SEND_BUFFER_SIZE);

    // Initialize Winsock
    int err = WSAStartup(MAKEWORD(2,2), &globs.wsaData);
    if (err != 0) {
        printf("WSAStartup failed: %d\n", err);
        ASSERT(false);
    }
}

// NOTE: keep chars allocated for ever
void nets_setTargetIp(str s) {
    globs.targetIp = s;
    globs.connected = false;
    _nets_sockCloseFree(&globs.simSocket);
}

bool nets_getConnected() { return globs.connected; }



// returns nullptr if failed, else ptr to data & increments current
U8* _nets_getBytes(U8* buf, U32 bufSize, U8** current, U32 count) {

    ASSERT(*current >= buf);
    if((*current) + count > buf + bufSize) { return nullptr; }
    U8* data = (*current);
    *current += count;
    return data;
}

/*
Message layout:
[U8 kind] [U8 length of name] [U8 data type] [U8 data size] [name] [data]
info is always big endian
*/
constexpr bool _nets_isSystemBigEndian()
{
    union {
        U32 i;
        char c[4];
    } x = {0x01020304};
    return x.c[0] == 1;
}

// NOTE: unsafe to use with overlapping src and dst
U8* _nets_reverseBytes(U8* data, U32 size, U8* dest) {
    for(int i = 0; i < size; i++) {
        dest[i] = data[size - 1 - i]; }
    return dest;
}

// formats and copies message into globs send buffer
void nets_putMessage(net_Prop message, BumpAlloc* scratch) {

    U32 bufSize = 4; // header
    bufSize += message.name.length;

    U32 dataLen = 0;
    if(message.type == net_propType_S32) { dataLen = sizeof(S32); }
    else if(message.type == net_propType_F64) { dataLen = sizeof(F64); }
    else if(message.type == net_propType_BOOL) { dataLen = sizeof(U8); }
    else if(message.type == net_propType_STR) { dataLen = message.data.str.length; }
    else { ASSERT(false); }
    bufSize += dataLen;

    U8* buffer = BUMP_PUSH_ARR(&globs.sendArena, bufSize, U8);
    buffer[0] = message.event?1:0;
    ASSERT(message.name.length < 256);
    buffer[1] = message.name.length;
    buffer[2] = message.type;
    buffer[3] = dataLen;
    str_copy(message.name, &(buffer[4]));

    U8* dataLoc = &(buffer[4 + message.name.length]);
    if(message.type == net_propType_S32) { *(S32*)(dataLoc) = message.data.s32; }
    else if(message.type == net_propType_F64) { *(F64*)(dataLoc) = message.data.f64; }
    else if(message.type == net_propType_BOOL) { *dataLoc = message.data.boo; }
    else if(message.type == net_propType_STR) { str_copy(message.data.str, dataLoc); }
    else { ASSERT(false); }


    // NOTE: IF ADDING TYPES THAT HAVE DYNAMIC SIZING/IGNORE BYTE ORDER FROM SYSTEM CHANGE THIS
    // VVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVV
    if(message.type != net_propType_STR) {
        if(!_nets_isSystemBigEndian()) {
            U8* n = _nets_reverseBytes(dataLoc, dataLen, BUMP_PUSH_ARR(scratch, dataLen, U8));
            memcpy(dataLoc, n, dataLen);
        }
    }
}

// UTILS TO MAKE MESSAGES FOR PUTTING
void net_putMessage(str name, F64 data, BumpAlloc* scratch) {
    net_Prop p;
    p.event = false;
    p.name = name;
    p.type = net_propType_F64;
    p.data.f64 = data;
    nets_putMessage(p, scratch);
}






// takes message info and constructs a prop inside of res, returns ptr to that prop
net_Prop* _nets_makeMessage(U8 isEvent, str name, U8* data, U8 dataType, U32 dataSize, BumpAlloc* scratch, BumpAlloc* res) {

    if(isEvent != 0 && isEvent != 1) { return nullptr; }
    if(dataType != net_propType_S32 &&
       dataType != net_propType_F64 &&
       dataType != net_propType_STR &&
       dataType != net_propType_BOOL) {
        return nullptr;
    }

    net_Prop* prop = BUMP_PUSH_NEW(res, net_Prop);
    prop->name = str_copy(name, res);
    prop->type = (net_PropType)dataType;
    prop->event = isEvent;


    bool flipWithEndiannes = false;
         if(prop->type == net_propType_F64) { flipWithEndiannes = true; }
    else if(prop->type == net_propType_S32) { flipWithEndiannes = true; }
    else if(prop->type == net_propType_BOOL) { flipWithEndiannes = false; }
    else if(prop->type == net_propType_STR) {
        prop->data.str = { (const U8*)data, dataSize };
        prop->data.str = str_copy(prop->data.str, BUMP_PUSH_ARR(res, dataSize, U8));
        return prop;
    }


    if(flipWithEndiannes && !_nets_isSystemBigEndian()) { _nets_reverseBytes(data, dataSize, (U8*)(&prop->data)); }
    else { memcpy((U8*)(&prop->data), data, dataSize); }
    return prop;
}


// NOTE: messages are expected to be formatted correctly, if one isn't this will not be able to read future messages
// parses out sections of a message, calls _netMakeMessage() to copy info into res
// outFirstProp is set to a pointer to the beginning of a ll with all parsed props
// return value indicates how much of the buffer was processed
U32 _net_processPackets(U8* buf, U32 bufSize, BumpAlloc* scratch, BumpAlloc* res, net_Prop** outFirstProp) {

    U8* cur = buf;
    while(true) {

        U8* cCopy = cur;
        U8* messageHeader = _nets_getBytes(buf, bufSize, &cCopy, 4);
        if(!messageHeader) { break; }

        U8 msgKind = messageHeader[0];
        U8 nameLen = messageHeader[1];
        U8 valType = messageHeader[2];
        U8 valLen = messageHeader[3];

        U8* nameBuf = _nets_getBytes(buf, bufSize, &cCopy, nameLen);
        if(!nameBuf) { break; }
        U8* dataBuf = _nets_getBytes(buf, bufSize, &cCopy, valLen);
        if(!dataBuf) { break; }


        net_Prop* p = _nets_makeMessage(msgKind, {nameBuf, nameLen}, dataBuf, valType, valLen, scratch, res);
        p->next = *outFirstProp;
        *outFirstProp = p;

        cur = cCopy;
    }

    return cur - buf;
}

net_Frame* nets_update(BumpAlloc* scratch, BumpAlloc* res, net_Table* table, float curTime) {
    nets_SockErr err;

    bool wasConnected = globs.connected;
    if(!globs.connected) {
        bool connected = _nets_sockCreateConnect(str_cstyle(globs.targetIp, scratch), NET_PORT, &globs.simSocket, &err);

        if(err != net_sockErr_none) { // attempt reconnection forever on errors
            _nets_sockCloseFree(&globs.simSocket); }
        else if(connected) {
            globs.connected = true;

            // str s = STR("[CONNECTED]\n");
            // fwrite(s.chars, 1, s.length, globs.logFile);
        }
    }


    // SEND LOOP ///////////////////////////////////////////////////////////////////////////

    if(globs.lastSendTime + NET_SEND_INTERVAL < curTime) {
        globs.lastSendTime = curTime;

        U8* start = (U8*)globs.sendArena.start;
        U8* end = (U8*)globs.sendArena.end;
        while (start < end && globs.connected) {
            // TODO: sent message logging
            int res = send(globs.simSocket.s, (const char*)start, min(end - start, NET_PACKET_SIZE), 0);
            if(res == SOCKET_ERROR) {
                if(WSAGetLastError() != WSAEWOULDBLOCK) {

                    printf("[SEND ERR] %d\n", WSAGetLastError());
                    // fwrite(s.chars, 1, s.length, globs.logFile);
                    // TODO: log local events
                    // TODO: logging

                    globs.connected = false;
                    _nets_sockCloseFree(&globs.simSocket);
                }
            }
            else { start += res; }
        }

        bump_clear(&globs.sendArena);
    }

    // RECV LOOP ///////////////////////////////////////////////////////////////////////////

    // NOTE: this is acting as an if, loop is normally broken when there is no more left to recieve
    net_Prop* framePropHead = nullptr;
    while(globs.connected) {

        U8* buf = (globs.recvBuffer + globs.recvBufStartOffset);
        int recvSize = recv(globs.simSocket.s, (char*)buf, NET_PACKET_SIZE, 0);

        if (recvSize == SOCKET_ERROR) {
            if(WSAGetLastError() != WSAEWOULDBLOCK) {
                printf("[RECV ERR] %d\n", WSAGetLastError());
                globs.connected = false;
                _nets_sockCloseFree(&globs.simSocket);
            }
            break;
        }
        // size is 0, indicating shutdown
        else if (recvSize == 0) {
            globs.connected = false;
            _nets_sockCloseFree(&globs.simSocket);
        }
        // buffer received
        else if (recvSize > 0) {

            U32 bufSize = globs.recvBufStartOffset + recvSize;
            U32 consumed = _net_processPackets(globs.recvBuffer, bufSize, scratch, res, &framePropHead);

            if(consumed > 0) {
                U8* remainder = globs.recvBuffer + consumed;
                U32 remSize = (bufSize - consumed);
                memmove(globs.recvBuffer, remainder, remSize);
                globs.recvBufStartOffset = remSize;
            }
            else { break; }
        }
    }

    if(!globs.connected && wasConnected) {
        printf("[DISCONNECTED]\n");
    }




    // combine LL of props into frame
    net_Frame* frame = BUMP_PUSH_NEW(res, net_Frame);
    frame->propCount = 0;
    frame->props = (net_Prop**)res->end;

    while(framePropHead) {
        frame->propCount++;
        net_Prop** p = (net_Prop**)bump_push(res, sizeof(net_Prop*));
        *p = framePropHead;
        framePropHead = framePropHead->next;
    }

    return frame;
}

void nets_cleanup() {
    printf("[QUIT]\n");
    _nets_sockCloseFree(&globs.simSocket);
    WSACleanup();
}



#endif