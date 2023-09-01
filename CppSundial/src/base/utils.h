#pragma once

#include "base/typedefs.h"
#include "base/config.h"
#include "base/allocators.h"


// first, last, and n should all be ptrs to the node struct
// uses prop "next" as link ptr
#define SLL_APPEND(first, last, n) \
    if(!first) { \
        first = n; \
        last = n; \
    } \
    else { \
        last->next = n; \
        last = n; \
    } \


#define ARR_APPEND(arr, count, elem) ((arr)[(count)] = (elem), &((arr)[(count)++]));
#define ARR_POP(arr, count) ((count)--, (arr)[(count)])
void* arr_allocate0(U64 elemSize, U64 count);

#define MIN(a, b) (((a) < (b))? (a) : (b))
#define MAX(a, b) (((a) > (b))? (a) : (b))



#ifdef BASE_IMPL

#include <stdio.h>
#include <memory>

void* arr_allocate0(U64 elemSize, U64 count) {

    U64 size = count * sizeof(elemSize);
    void* v = malloc(size);
    ASSERT(v);
    memset(v, 0, size);
    return v;

}


U8* loadFileToBuffer(const char* path, bool asText, U64* outSize, BumpAlloc* arena) {
    if(outSize) { *outSize = 0; }

    const char* openStr = (asText)? "r" : "rb";
    FILE* file = fopen(path, openStr);
    if(file == NULL) { return nullptr; }

    fseek(file, 0L, SEEK_END);
    U64 size = ftell(file);
    fseek(file, 0L, SEEK_SET);

    U8* text = BUMP_PUSH_ARR(arena, size, U8);

    fread(text, sizeof(U8), size, file);
    fclose(file);

    if(outSize) { *outSize = size; }
    return text;
}


#endif