#pragma once

#include "typedefs.h"
struct BumpAlloc;





struct str {
    const U8* chars;
    U64 length;
};

#define STR(x) str_make(x)

str str_make(const char* c); // use given memory
str str_make(const char* c, BumpAlloc& arena); // copy given cstr to the heap
str str_copy(str a, BumpAlloc& arena);
str str_join(str l, str r, BumpAlloc& arena);
str str_substr(str s, U64 start, U64 length);
const char* str_cstyle(str s, BumpAlloc& arena);
bool str_compare(str a, str b);

void str_print(str s);
void str_println(str s);
void str_printf(str fmt, ...);

// TODO: float conversion magic



#ifdef BASE_IMPL

#include "allocators.h"
#include <string.h>
#include <memory.h>
#include <stdio.h>
#include <stdarg.h>
#include <cmath>
#include "arr.h"

void str_printf(str fmt, ...) {
    va_list argp;
    va_start(argp, fmt);

    for(const U8* ptr = fmt.chars; ptr < (fmt.chars + fmt.length); ptr++) {

        if(*ptr == '%'){
            ptr++;

            if(*ptr == 's') {
                str st = va_arg(argp, str);
                for(const U8* i = st.chars; i < st.chars + st.length; i++) {
                    putchar(*i);
                }
            }
            else if(*ptr == 'i') {
                int i = va_arg(argp, int);
                if(i == 0) { putchar('0'); }
                else if(i < 0) {
                    putchar('-');
                    i = -i;
                }

                U8 mem[32] = { };
                int digit = 0;

                while(i > 0) {
                    ARR_APPEND(mem, digit, '0' + (i%10));
                    i /= 10;
                }
                while(U8 x = ARR_POP(mem, digit)) { putchar(x); }
            }
            else {
                putchar(*ptr); }
        }
        else {
            putchar(*ptr);
        }


    }
    va_end(argp);
}



str str_make(const char* c, BumpAlloc& arena) {

    U64 len = strlen(c);
    U8* a = BUMP_PUSH_ARR(arena, len, U8);
    memcpy(a, c, len);
    return { a, len };
}

str str_make(const char* c) {
    const U8* n = reinterpret_cast<const U8*>(c);
    U64 len = strlen(c);
    return { n, len };
}

str str_copy(str a, BumpAlloc& arena) {

    U8* n = BUMP_PUSH_ARR(arena, a.length, U8);
    memcpy(n, a.chars, a.length);
    return { n, a.length };
}

str str_join(str l, str r, BumpAlloc& arena) {
    U64 len = l.length + r.length;
    U8* n = BUMP_PUSH_ARR(arena, len, U8);
    memcpy(n, l.chars, l.length);
    memcpy(n + l.length, r.chars, r.length);
    return { n, len };
}

str str_substr(str s, U64 start, U64 length) {
    DEBUG_ASSERT(start < s.length);
    DEBUG_ASSERT(length <= (s.length - start));
    return { s.chars + start, length };
}

// NOTE: performs an arena allocation and copy because of the null terminator
const char* str_cstyle(str s, BumpAlloc& arena) {

    char* n = BUMP_PUSH_ARR(arena, s.length + 1, char);
    memcpy(n, s.chars, s.length);
    n[s.length] = '\0';
    return n;
}

bool str_compare(str a, str b)
{
    if(a.length != b.length) { return false; }
    return memcmp(a.chars, b.chars, a.length) == 0;
}

void str_print(str s) {

    const U8* ptr = s.chars;
    for(; ptr < (s.chars + s.length); ptr++) {
        putchar(*ptr); }
}

void str_println(str s) {

    str_print(s);
    putchar('\n');
}

#endif