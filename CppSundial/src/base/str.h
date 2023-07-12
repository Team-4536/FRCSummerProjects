#pragma once

#include "typedefs.h"
struct BumpAlloc;





struct str {
    const U8* chars;
    U64 length;
};
struct StrNode {
    str s = { nullptr, 0 };
    StrNode* next = nullptr;
};
struct StrList {
    StrNode* start = nullptr;
    StrNode* end = nullptr;
};


#define STR(x) str{ (const U8*)x, sizeof(x)-1 } // -1 for null term
// #define STR(x) str_make(x)

str str_make(const char* c); // use given memory
str str_make(const char* c, BumpAlloc* arena); // copy c into arena
str str_copy(str a, BumpAlloc* arena);
str str_copy(str a, U8* memory);
str str_join(str l, str r, BumpAlloc* arena);
str str_substr(str s, U64 start, U64 length);
const char* str_cstyle(str s, BumpAlloc* arena);
bool str_compare(str a, str b);

void str_print(str s);
void str_println(str s);
void str_printf(str fmt, ...);

str str_format(BumpAlloc* arena, str fmt, ...);



// copies string into arena, and appends a new node (in arena) to list.
void str_listAppend(StrList* list, str string, BumpAlloc* arena);

// copies all strings in s into arena, returns one str that goes over all
str str_listCollect(StrList s, BumpAlloc* arena);
// str str_listCollect(StrList s, str sep, BumpAlloc* arena);

#ifdef BASE_IMPL

#include "allocators.h"
#include <string.h>
#include <memory.h>
#include <stdio.h>
#include <stdarg.h>
#include <cmath>
#include "arr.h"


// CLEANUP: reuse this code,
// CLEANUP: bumpalloc default clear could slow it down a ton
str str_format(BumpAlloc* arena, str fmt, ...) {
    va_list argp;
    va_start(argp, fmt);

    str out = { (const U8*)arena->end, 0 };

    for(const U8* ptr = fmt.chars; ptr < (fmt.chars + fmt.length); ptr++) {

        if(*ptr == '%'){
            ptr++;

            if(*ptr == 's') {
                str st = va_arg(argp, str);
                for(const U8* i = st.chars; i < st.chars + st.length; i++) {
                    *BUMP_PUSH_NEW(arena, char) = *i;
                    out.length++;
                }
            }
            else if(*ptr == 'i') {
                int i = va_arg(argp, int);
                if(i == 0) {
                    *BUMP_PUSH_NEW(arena, char) = '0';
                    out.length++;
                }
                else if(i < 0) {
                    *BUMP_PUSH_NEW(arena, char) = '-';
                    out.length++;
                    i = -i;
                }

                U8 mem[32] = { };
                int digit = 0;

                while(i > 0) {
                    ARR_APPEND(mem, digit, '0' + (i%10));
                    i /= 10;
                }
                while(U8 x = ARR_POP(mem, digit)) {
                    *BUMP_PUSH_NEW(arena, char) = x;
                    out.length++;
                }
            }
            else if(*ptr == 'f') {
                char buf[10] = { 0 };
                if(gcvt(va_arg(argp, double), 6, buf) == NULL) { continue; };
                for(int i = 0; i < 10; i++) {
                    if(buf[i] == '\0') { break; }
                    *BUMP_PUSH_NEW(arena, char) = buf[i];
                    out.length++;
                }
            }
            else if(*ptr == 'b') {
                str boolStr = va_arg(argp, int)? STR("true") : STR("false");
                str_copy(boolStr, arena);
                out.length += boolStr.length;
            }
            else {
                *BUMP_PUSH_NEW(arena, char) = *ptr;
                out.length++;
            }
        }
        else {
            *BUMP_PUSH_NEW(arena, char) = *ptr;
            out.length++;
        }


    }
    va_end(argp);

    return out;
}

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
            else if(*ptr == 'f') {
                char buf[10];
                if(gcvt(va_arg(argp, double), 6, buf) == NULL) { continue; };
                for(int i = 0; i < 10; i++) {
                    putchar(buf[i]);
                    if(buf[i] == '\0') { break; }
                }
            }
            else if(*ptr == 'b') {
                str boolStr = va_arg(argp, int)? STR("true") : STR("false");
                for(int i = 0; i < boolStr.length; i++) {
                    putchar(boolStr.chars[i]);
                }
            }
            else {
                putchar(*ptr);
            }
        }
        else {
            putchar(*ptr);
        }


    }
    va_end(argp);
}



str str_make(const char* c, BumpAlloc* arena) {

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

str str_copy(str a, BumpAlloc* arena) {

    U8* n = BUMP_PUSH_ARR(arena, a.length, U8);
    memcpy(n, a.chars, a.length);
    return { n, a.length };
}

str str_copy(str a, U8* memory) {
    memcpy(memory, a.chars, a.length);
    return { memory, a.length };
}

str str_join(str l, str r, BumpAlloc* arena) {
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
const char* str_cstyle(str s, BumpAlloc* arena) {

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





void str_listAppend(StrList* list, str string, BumpAlloc* arena) {

    StrNode* node = BUMP_PUSH_NEW(arena, StrNode);
    node->s = str_copy(string, arena);

    if(!list->start) {
        list->start = node;
        list->end = node;
    }
    else {
        list->end->next = node;
        list->end = node;
    }
}

str str_listCollect(StrList s, BumpAlloc* arena) {
    str out = { (U8*)arena->end, 0 };

    StrNode* n = s.start;
    while(n) {
        str copy = str_copy(n->s, arena);
        out.length += n->s.length;
        n = n->next;
    }

    return out;
}


/*
str str_listCollect(StrList s, str sep, BumpAlloc* arena) {
    str out = { (U8*)arena->end, 0 };

    StrNode* n = s.start;
    while(n) {
        str copy = str_copy(n->s, arena);
        str_copy(sep, arena);
        out.length += n->s.length + sep.length;
        n = n->next;
    }

    return out;
}
*/

#endif