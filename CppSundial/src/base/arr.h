#pragma once
#include "typedefs.h"
#include "config.h"



// #define ARR_APPEND(arr, count, elem) do { (arr)[(count)] = (elem); (count)++; } while(0)
#define ARR_APPEND(arr, count, elem) ((arr)[(count)] = (elem), &((arr)[(count)++]));
#define ARR_POP(arr, count) ((count)--, (arr)[(count)])


void* arr_allocate0(U64 elemSize, U64 count);



#ifdef BASE_IMPL

void* arr_allocate0(U64 elemSize, U64 count) {

    U64 size = count * sizeof(elemSize);
    void* v = malloc(size);
    ASSERT(v);
    memset(v, 0, size);
    return v;

}

#endif