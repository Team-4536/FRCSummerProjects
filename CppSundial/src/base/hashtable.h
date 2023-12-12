#pragma once
#include "typedefs.h"
#include "str.h"
#include "config.h"



// returns a hash value for the given string
U64 hash_hashStr(str x);






#ifdef BASE_IMPL

// stolen, https://stackoverflow.com/questions/7666509/hash-function-for-string
U64 hash_hashStr(str s) {

    U64 hash = 5381;
    const U8* ptr = s.chars;
    U8 c = *ptr;
    for (; ptr < (s.chars + s.length); c = *++ptr) {
        hash = ((hash << 5) + hash) + c; /* hash * 33 + c */
    }

    return hash;
}

#endif


