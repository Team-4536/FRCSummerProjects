#pragma once

#include "base/str.h"
#include "base/typedefs.h"
#include "base/allocators.h"
#include "network.h"





void nets_init(BumpAlloc* scratch, BumpAlloc* res);
void nets_cleanup();


// new info is added to table when recieved
// data points allocated into res arena
void nets_update(net_Table* table, float curTime, str targetIp);

void nets_putMessage(str name, F64 data);
void nets_putMessage(str name, S32 data);
void nets_putMessage(str name, str data);
void nets_putMessage(str name, bool data);


#ifdef NET_IMPL

#include <stdio.h>
#include <winsock2.h>
#include <WS2tcpip.h>
#include <iphlpapi.h>

#include "base/hashtable.h"
#include "base/utils.h"

#define NETS_PACKET_SIZE 1024
#define NETS_RECV_BUFFER_SIZE 2048
#define NETS_SEND_BUFFER_SIZE 100000

#define NETS_SEND_INTERVAL (1/50.0f)
#define NETS_PORT "7000"
#define NETS_MAX_SAMPLE_COUNT 200





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

    net_PropSample* firstFreeSample = nullptr;
    BumpAlloc* scratch = nullptr;
    BumpAlloc* res = nullptr;

    FILE* logFile = nullptr;
};

static nets_Globs globs = nets_Globs();

// Asserts on failures
void nets_init(BumpAlloc* scratch, BumpAlloc* res) {

    globs.recvBuffer = (U8*)arr_allocate0(1, NETS_RECV_BUFFER_SIZE);
    bump_allocate(&globs.sendArena, NETS_SEND_BUFFER_SIZE);

    // Initialize Winsock
    int err = WSAStartup(MAKEWORD(2,2), &globs.wsaData);
    if (err != 0) {
        printf("WSAStartup failed: %d\n", err);
        ASSERT(false);
    }

    globs.scratch = scratch;
    globs.res = res;
    globs.logFile = fopen("sunLog.log", "wb");
}



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






struct nets_Message {
    str name;
    net_PropType type;

    union {
        F64 f64;
        S32 s32;
        str str;
        U8 boo;
    };
};
// formats and copies message into globs send buffer
void nets_putMessage(nets_Message message) {

    U32 bufSize = 4; // header
    bufSize += message.name.length;

    U32 dataLen = 0;
    if(message.type == net_propType_S32) { dataLen = sizeof(S32); }
    else if(message.type == net_propType_F64) { dataLen = sizeof(F64); }
    else if(message.type == net_propType_BOOL) { dataLen = sizeof(U8); }
    else if(message.type == net_propType_STR) { dataLen = message.str.length; }
    else { ASSERT(false); }
    bufSize += dataLen;

    U8* buffer = BUMP_PUSH_ARR(&globs.sendArena, bufSize, U8);
    buffer[0] = 0; // TODO: events
    ASSERT(message.name.length < 256);
    buffer[1] = message.name.length;
    buffer[2] = message.type;
    ASSERT(dataLen < 256);
    buffer[3] = dataLen;
    str_copy(message.name, &(buffer[4]));

    U8* dataLoc = &(buffer[4 + message.name.length]);
    if(message.type == net_propType_S32) { *(S32*)(dataLoc) = message.s32; }
    else if(message.type == net_propType_F64) { *(F64*)(dataLoc) = message.f64; }
    else if(message.type == net_propType_BOOL) { *dataLoc = message.boo; }
    else if(message.type == net_propType_STR) { str_copy(message.str, dataLoc); }
    else { ASSERT(false); }


    // NOTE: IF ADDING TYPES THAT HAVE DYNAMIC SIZING/IGNORE BYTE ORDER FROM SYSTEM CHANGE THIS
    // VVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVV
    if(message.type != net_propType_STR) {
        if(!_nets_isSystemBigEndian()) {
            U8* n = _nets_reverseBytes(dataLoc, dataLen, BUMP_PUSH_ARR(globs.scratch, dataLen, U8));
            memcpy(dataLoc, n, dataLen);
        }
    }
}



// TODO: send buffering
void nets_putMessage(str name, F64 data) {
    nets_Message p;
    p.name = name;
    p.type = net_propType_F64;
    p.f64 = data;
    nets_putMessage(p);
}
void nets_putMessage(str name, S32 data) {
    nets_Message p;
    p.name = name;
    p.type = net_propType_S32;
    p.s32 = data;
    nets_putMessage(p);
}
void nets_putMessage(str name, str data) {
    nets_Message p;
    p.name = name;
    p.type = net_propType_STR;
    p.str = data;
    nets_putMessage(p);
}
void nets_putMessage(str name, bool data) {
    nets_Message p;
    p.name = name;
    p.type = net_propType_BOOL;
    p.boo = data?1:0;
    nets_putMessage(p);
}





// returns nullptr on invalid type add
// allocates new prop names into res
net_PropSample* _nets_registerSample(net_Table* table, str propName, net_PropType type) {
    net_Prop* prop = net_getProp(propName, table);
    if(prop && (prop->type != type)) {
        return nullptr; }
    else if(!prop) {
        prop = ARR_APPEND(table->props, table->propCount, net_Prop());
        prop->name = str_copy(propName, globs.res);
        prop->type = type;

        StrList log = StrList();
        str_listAppend(&log, STR("p "), globs.scratch);
        str_listAppend(&log, prop->name, globs.scratch);
        str_listAppend(&log, STR(" "), globs.scratch);
        str s;
        if(prop->type == net_propType_S32) { s = STR("s32"); }
        else if(prop->type == net_propType_F64) { s = STR("f64"); }
        else if(prop->type == net_propType_BOOL) { s = STR("bool"); }
        else if(prop->type == net_propType_STR) { s = STR("str"); }
        else { ASSERT(false); }
        str_listAppend(&log, s, globs.scratch);
        str_listAppend(&log, STR("\n"), globs.scratch);
        str fin = str_listCollect(log, globs.scratch);
        fwrite(fin.chars, 1, fin.length, globs.logFile);
    }


    net_PropSample* sample = globs.firstFreeSample;
    if(sample) {
        globs.firstFreeSample = sample->nextFree;
    }
    else {
        sample = BUMP_PUSH_NEW(globs.res, net_PropSample);
    }

    sample->next = prop->firstPt;
    if(prop->firstPt) { prop->firstPt->prev = sample; }
    else { prop->lastPt = sample; }
    prop->firstPt = sample;
    prop->ptCount++;

    // start freeing samples if they are too old
    if(prop->ptCount > NETS_MAX_SAMPLE_COUNT) {
        prop->lastPt->nextFree = globs.firstFreeSample;
        globs.firstFreeSample = prop->lastPt;

        prop->lastPt->prev->next = nullptr;
        prop->lastPt = prop->lastPt->prev;
        prop->ptCount--;
    }

    return sample;
}

// TODO: make better
void _nets_logUpdate(net_Prop* prop) {
    net_PropSample* sample = prop->firstPt;

    StrList entry = StrList();
    str_listAppend(&entry, STR("u "), globs.scratch);
    str_listAppend(&entry, prop->name, globs.scratch);
    str_listAppend(&entry, STR(" "), globs.scratch);
    str_listAppend(&entry, str_format(globs.scratch, STR("%f "), sample->timeStamp), globs.scratch);

    if(prop->type == net_propType_S32) { str_listAppend(&entry, str_format(globs.scratch, STR("%i"), sample->s32), globs.scratch); }
    else if(prop->type == net_propType_F64) { str_listAppend(&entry, str_format(globs.scratch, STR("%f"), sample->f64), globs.scratch); }
    else if(prop->type == net_propType_BOOL) { str_listAppend(&entry, str_format(globs.scratch, STR("%b"), sample->boo), globs.scratch); }
    else if(prop->type == net_propType_STR) { str_listAppend(&entry, sample->str, globs.scratch); }
    else { ASSERT(false); }

    str_listAppend(&entry, STR("\n"), globs.scratch);

    str fin = str_listCollect(entry, globs.scratch);
    fwrite(fin.chars, 1, fin.length, globs.logFile);
}



// takes message info and construct a sample inside of res
// appends sample to prop in table, if no prop a new one is created
// string and prop name char data also allocated in res, (TODO: this is a memory leak)
// does nothing on invalid messages and messages that have a different type than the one being used
void _nets_processMessage(U8 isEvent, str name, U8* data, U8 dataType, U32 dataSize, net_Table* table, float currentTime) {

    if(isEvent != 0) { return; }
    if(dataType != net_propType_S32 &&
       dataType != net_propType_F64 &&
       dataType != net_propType_STR &&
       dataType != net_propType_BOOL) {
        return;
    }


    net_PropSample* sample = _nets_registerSample(table, name, (net_PropType)dataType);
    if(!sample) { return; }
    sample->timeStamp = currentTime;

    bool flipWithEndiannes = false;
         if(dataType == net_propType_F64) { flipWithEndiannes = true; }
    else if(dataType == net_propType_S32) { flipWithEndiannes = true; }
    else if(dataType == net_propType_BOOL) { flipWithEndiannes = false; }
    else if(dataType == net_propType_STR) {
        sample->str = { (const U8*)data, dataSize };
        sample->str = str_copy(sample->str, BUMP_PUSH_ARR(globs.res, dataSize, U8));
        _nets_logUpdate(net_getProp(name, table));
        return;
    }

    // NOTE: this is very dangerous, but it works
    // data is kept first inside of sample struct
    if(flipWithEndiannes && !_nets_isSystemBigEndian()) { _nets_reverseBytes(data, dataSize, (U8*)(sample)); }
    else { memcpy((U8*)(sample), data, dataSize); }

    _nets_logUpdate(net_getProp(name, table));
}


// NOTE: messages are expected to be formatted correctly, if one isn't this will not be able to read future messages
// parses out sections of a message, calls _netProcessMessage() to create/append samples to table
// return value indicates how much of the buffer was processed
U32 _net_processPackets(U8* buf, U32 bufSize, net_Table* table, float currentTime) {

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

        _nets_processMessage(msgKind, {nameBuf, nameLen}, dataBuf, valType, valLen, table, currentTime);

        cur = cCopy;
    }

    return cur - buf;
}

void nets_update(net_Table* table, float curTime, str targetIp) {
    nets_SockErr err;
    bool wasConnected = globs.connected;


    bool tryConnection = false;


    if(!globs.connected) {
        if(globs.targetIp.length != 0) {
            tryConnection = true;
        }
    }

    if(!str_compare(targetIp, globs.targetIp)) {
        globs.connected = false; }

    if(tryConnection) {
        // retry connection forever if failed
        bool connected = _nets_sockCreateConnect(str_cstyle(targetIp, globs.scratch), NETS_PORT, &globs.simSocket, &err);

        if(err != net_sockErr_none) { _nets_sockCloseFree(&globs.simSocket); }
        else if(connected) {
            globs.connected = true;
            printf("[CONNECTED]\n");
            net_PropSample* s = _nets_registerSample(table, STR("/connected"), net_propType_BOOL);
            s->boo = true;
            s->timeStamp = curTime;
            _nets_logUpdate(net_getProp(STR("/connected"), table));
        }
    }
    globs.targetIp = targetIp;



    // SEND LOOP ///////////////////////////////////////////////////////////////////////////

    if(globs.lastSendTime + NETS_SEND_INTERVAL < curTime) {
        globs.lastSendTime = curTime;

        U8* start = (U8*)globs.sendArena.start;
        U8* end = (U8*)globs.sendArena.end;
        while (start < end && globs.connected) {
            // TODO: sent message logging
            int res = send(globs.simSocket.s, (const char*)start, MIN(end - start, NETS_PACKET_SIZE), 0);
            if(res == SOCKET_ERROR) {
                if(WSAGetLastError() != WSAEWOULDBLOCK) {

                    printf("[SEND ERR] %d\n", WSAGetLastError());
                    // TODO: log local events
                    // TODO: invalid message logging
                    globs.connected = false;
                }
            }
            else { start += res; }
        }

        bump_clear(&globs.sendArena);
    }

    // RECV LOOP ///////////////////////////////////////////////////////////////////////////

    // NOTE: this is acting as an if, loop is normally broken when there is no more left to recieve
    while(globs.connected) {

        U8* buf = (globs.recvBuffer + globs.recvBufStartOffset);
        int recvSize = recv(globs.simSocket.s, (char*)buf, NETS_PACKET_SIZE, 0);

        if (recvSize == SOCKET_ERROR) {
            if(WSAGetLastError() != WSAEWOULDBLOCK) {
                printf("[RECV ERR] %d\n", WSAGetLastError());
                globs.connected = false;
            }
            break;
        }
        // size is 0, indicating shutdown
        else if (recvSize == 0) {
            globs.connected = false;
        }
        // buffer received
        else if (recvSize > 0) {

            U32 bufSize = globs.recvBufStartOffset + recvSize;
            U32 consumed = _net_processPackets(globs.recvBuffer, bufSize, table, curTime);

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

        _nets_sockCloseFree(&globs.simSocket);

        net_PropSample* s = _nets_registerSample(table, STR("/connected"), net_propType_BOOL);
        s->boo = false;
        s->timeStamp = curTime;
        _nets_logUpdate(net_getProp(STR("/connected"), table));
        printf("[DISCONNECTED]\n");
    }

    // printf("%i\n", (U8*)globs.res.end - (U8*)globs.res.start);
}

void nets_cleanup() {
    printf("[QUIT]\n");
    fclose(globs.logFile);
    _nets_sockCloseFree(&globs.simSocket);
    WSACleanup();
}

#endif