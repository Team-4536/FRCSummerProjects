#pragma once
#include "typedefs.h"


// uses malloc and free on allocate and free
// fixed size
// zeroes memory on allocation, not on release (and not on construction)
struct BumpAlloc {
    void* start;
    void* end;
    U64 reserved;
};


BumpAlloc* bump_init(BumpAlloc* a, U64 size, void* memory);

BumpAlloc* bump_allocate(BumpAlloc* a, U64 size);
void bump_free(BumpAlloc* a);

// asserts on failure
void* bump_push(BumpAlloc* a, U64 size);

// asserts on failure
void bump_pop(BumpAlloc* a, U64 size);

void bump_clear(BumpAlloc* a);

// push new pushes sizeof(type) and initializes memory with default type ctor
// [bump] is expected to be a pointer
#define BUMP_PUSH_NEW(bump, type) &(*(static_cast<type*>(bump_push(bump, sizeof(type)))) = type())

// push arr pushes space for count objects of type, does not initialize memory beyond zeroing
// [bump] is expected to be a pointer
#define BUMP_PUSH_ARR(bump, count, type) static_cast<type*>(bump_push(bump, sizeof(type) * (count)))



#ifdef BASE_IMPL


#include "config.h"
#include <malloc.h>
#include <memory.h>


BumpAlloc* bump_init(BumpAlloc* a, U64 size, void* memory) {
    a->reserved = size;
    a->start = memory;
    a->end = a->start;
    return a;
}

BumpAlloc* bump_allocate(BumpAlloc* a, U64 size) {
    a->reserved = size;
    a->start = malloc(size);
    ASSERT(a->start);
    a->end = a->start;
    return a;
}

void bump_free(BumpAlloc* a) {
    a->reserved = 0;
    free(a->start);
    a->start = nullptr;
    a->end = nullptr;
}

void* bump_push(BumpAlloc* a, U64 size) {
    char* o = static_cast<char*>(a->end);
    ASSERT(o + size < static_cast<char*>(a->start) + a->reserved);
    a->end = o+size;
    memset(o, 0, size);
    return o;
}

void bump_pop(BumpAlloc* a, U64 size) {
    char* c = static_cast<char*>(a->end);
    ASSERT(size <= (c - static_cast<char*>(a->start)));
    a->end = c - size;
}

void bump_clear(BumpAlloc* a) {
    a->end = a->start;
    // TODO: 0 memory just in case
}

#endif