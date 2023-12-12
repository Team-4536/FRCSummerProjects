#pragma once
#include "typedefs.h"


// struct to represent a contiguous buffer, and an end pointer
// memory is reserved by moving the end pointer and returning the space that was there before
struct BumpAlloc {
    void* start;
    void* end;
    U64 reserved;
};


// basic ctor that fills in an arena based on a prexisting buffer
// don't use bump_free if bump_allocate was not used to reserve the buffer
BumpAlloc* bump_init(BumpAlloc* a, U64 size, void* memory);

// use malloc to reserve [size] amount of bytes for the arena
BumpAlloc* bump_allocate(BumpAlloc* a, U64 size);
// use free to release reserved buffer (start)
void bump_free(BumpAlloc* a);

// move end pointer in arena forward by size and return previous end
// asserts failure if end pointer moves outside of the reserved buffer
void* bump_push(BumpAlloc* a, U64 size);

// asserts on failure
// reduces used memory in arena by size bytes
void bump_pop(BumpAlloc* a, U64 size);

void bump_clear(BumpAlloc* a);

// push new pushes sizeof(type) and initializes memory with default type ctor
// [bump] is expected to be a pointer
#define BUMP_PUSH_NEW(bump, type) &(*(static_cast<type*>(bump_push(bump, sizeof(type)))) = type())

// push arr pushes space for count objects of type, does not initialize memory beyond zeroing
// [bump] is expected to be a pointer to a BumpAlloc
// [count] is a number
#define BUMP_PUSH_ARR(bump, count, type) static_cast<type*>(bump_push(bump, sizeof(type) * (count)))



#ifdef BASE_IMPL


#include "config.h"
#include <malloc.h>
#include <memory.h>


BumpAlloc* bump_init(BumpAlloc* a, U64 size, void* memory) {
    a->reserved = size;
    a->start = memory;
    bump_clear(a);
    return a;
}

BumpAlloc* bump_allocate(BumpAlloc* a, U64 size) {
    a->reserved = size;
    a->start = malloc(size);
    ASSERT(a->start);
    bump_clear(a);
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
    if(!(o + size < static_cast<char*>(a->start) + a->reserved)) {
        ASSERT(false);
    }
    a->end = o+size;
    return o;
}

void bump_pop(BumpAlloc* a, U64 size) {
    char* c = static_cast<char*>(a->end);
    ASSERT(size <= (c - static_cast<char*>(a->start)));
    a->end = c - size;
    memset(a->end, 0, size);
}

void bump_clear(BumpAlloc* a) {
    a->end = a->start;
    memset(a->start, 0, a->reserved);
}

#endif