#pragma once

// Sry for the bad name
// "Prop" refers to, roughly, network table entries
// Except that they also represent messages
// but message is taken to refer to the encoded version of props

#include "base/str.h"

enum net_PropType {
    net_propType_S32,
    net_propType_F64,
    net_propType_STR,
    net_propType_BOOL,
    net_propType_NONE
};

union net_PropData {
    F64 f64;
    S32 s32;
    str str;
    U8 boo;
};

struct net_Prop {
    str name;
    net_PropType type;
    net_PropData data;

    bool event;

    net_Prop* next; // for LL based creation
};

struct net_Frame {
    U32 propCount;
    net_Prop** props;
};


#define NET_FRAME_COUNT 300
struct net_Table {
    net_Frame* frames[NET_FRAME_COUNT];
};


net_Prop* net_getProp(str name, net_Frame* frame);


#ifdef NET_IMPL
#include "base/hashtable.h"

net_Prop* net_getProp(str name, net_Frame* frame) {
    U64 hashKey = hash_hashStr(name);


    for(int i = 0; i < frame->propCount; i++) {
        net_Prop* p = frame->props[i];
        // PERF: this is bad
        U64 pKey = hash_hashStr(p->name);
        if(hashKey == pKey) { return p; }
    }
    return nullptr;
}

#endif