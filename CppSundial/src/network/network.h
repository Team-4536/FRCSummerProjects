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
// TODO: update socket code to work w none type

struct net_PropSample {
    union {
        F64 f64;
        S32 s32;
        str str;
        U8 boo;
    };
    float timeStamp = 0; // client side timestamp
    net_PropSample* next = nullptr; // further back in time

    // used to track allocation and clean up when there is a live connection
    net_PropSample* prev = nullptr; // forward in time
    net_PropSample* nextFree = nullptr;
};

// data held in a double linked list
struct net_Prop {
    str name;
    net_PropType type;
    // safe to assume that firstPt will never be nullptr if the prop exists
    net_PropSample* firstPt = nullptr; // most recent
    net_PropSample* lastPt = nullptr; // oldest
    U32 ptCount = 0;
};



#define NET_MAX_PROP_COUNT 300
struct net_Table {
    net_Prop props[NET_MAX_PROP_COUNT] = { 0 };
    U32 propCount = 0;
    // TODO: convert to hash table
    // NOTE: table should only contain one prop with a given name, even if you want two with different types

    // TODO: events
};

net_Prop* net_getProp(str name, net_Table* table);
net_Prop* net_getProp(str name, net_PropType type, net_Table* table);

// returns nullptr on failed get
net_PropSample* net_getSample(str name, net_PropType type, float sampleTime, net_Table* table);
// returns first sample, nullptr if prop does not exist
net_PropSample* net_getSample(str name, net_PropType type, net_Table* table);



#ifdef NET_IMPL
#include "base/hashtable.h"


net_PropSample* net_getSample(str name, net_PropType type, net_Table* table) {
    net_Prop* p = net_getProp(name, type, table);
    if(!p) { return nullptr; }
    return p->firstPt;
}

net_PropSample* net_getSample(str name, net_PropType type, float sampleTime, net_Table* table) {

    net_Prop* p = net_getProp(name, type, table);
    if(!p) { return nullptr; }

    net_PropSample* sample = p->firstPt;
    while(sample) {
        if(sample->timeStamp < sampleTime) { break; }
        sample = sample->next;
    }
    return sample;
}

net_Prop* net_getProp(str name, net_PropType type, net_Table* table) {
    net_Prop* p = net_getProp(name, table);
    if(!p) { return nullptr; }
    else if(p->type != type) { return nullptr; }
    return p;
}

net_Prop* net_getProp(str name, net_Table* table) {
    U64 hashKey = hash_hashStr(name);

    for(int i = 0; i < table->propCount; i++) {
        net_Prop* p = &table->props[i];
        U64 pKey = hash_hashStr(p->name);
        if(hashKey == pKey) {
            return p;
        }
    }
    return nullptr;
}

#endif