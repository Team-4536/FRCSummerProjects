#pragma once

#ifndef UTILS_INCLUDED
#define UTILS_INCLUDED


#include "base/config.h"
#include "base/typedefs.h"

#include <math.h>
#include <stdio.h>



// TODO: make utils into a single header

#define min(a, b) ((a)<(b)?(a):(b))
#define max(a, b) ((a)>(b)?(a):(b))





F32 lerp(F32 a, F32 b, F32 t);



struct V2i {

    S32 x = 0;
    S32 y = 0;

    V2i() { }
    V2i(S32 x, S32 y): x(x), y(y) { }


    inline V2i operator+(const V2i& b) const {
        return { this->x + b.x, this->y + b.y }; }

    inline V2i operator+() const {
        return *this; }

    inline V2i operator-(const V2i& b) const {
        return { this->x - b.x, this->y - b.y }; }

    inline V2i operator-() const {
        return { -this->x, -this->y }; }

};


struct V2f {

    F32 x = 0;
    F32 y = 0;

    V2f() { }
    V2f(F32 x, F32 y): x(x), y(y) { }
    V2f(V2i b): x(b.x), y(b.y) { }



    inline V2f operator+(const V2f& b) const {
        return { this->x + b.x, this->y + b.y }; }

    inline V2f operator+() const {
        return *this; }

    inline V2f operator-(const V2f& b) const {
        return { this->x - b.x, this->y - b.y }; }

    inline V2f operator-() const {
        return { -this->x, -this->y }; }

    F32 length() const {
        return sqrtf(this->x * this->x + this->y * this->y);
    }

    F32 lengthSquared() const {
        return this->x * this->x + this->y * this->y;
    }

};




struct V4f {

    F32 x = 0;
    F32 y = 0;
    F32 z = 0;
    F32 w = 1;

    V4f() { }
    V4f(F32 x, F32 y, F32 z, F32 w): x(x), y(y), z(z), w(w) { }


    // CLEANUP: These only work with vector as lhs
    inline V4f operator+(const V4f& b) const {
        return { this->x + b.x, this->y + b.y, this->z + b.z, this->w + b.w }; }

    inline V4f operator+() const {
        return *this; }

    inline V4f operator-(const V4f& b) const {
        return { this->x - b.x, this->y - b.y, this->z - b.z, this->w - b.w }; }

    inline V4f operator-() const {
        return { -this->x, -this->y, -this->z, -this->w  }; }


    // this really needs to be less awful, but c++ is a terrible language
    F32& operator[](U32 i) {
        switch (i)
        {
        case 0: return this->x; break;
        case 1: return this->y; break;
        case 2: return this->z; break;
        case 3: return this->w; break;
        default: assert(false); break;
        }
    }
};

V4f v4f_lerp(V4f a, V4f b, F32 t);

F32 v2fDot(const V2f& a, const V2f& b);
V2f v2fNormalized(const V2f& a); // NOTE: is it good practice to const ref a small struct like vectors?


inline void operator-=(V4f& a, const V4f& b) { a = a - b; }
inline void operator+=(V4f& a, const V4f& b) { a = a + b; }

// CLEANUP: V4 operators not done lol
inline V4f operator*(const V4f& a, F32 b) {
    return { a.x * b, a.y * b, a.z * b, a.w * b }; }

inline V4f operator*(F32 b, const V4f& a) {
    return { a.x * b, a.y * b, a.z * b, a.w * b }; }

inline V4f operator/(const V4f& a, F32 b) {
    return { a.x / b, a.y / b, a.z / b, a.w / b }; }

inline V4f operator/(F32 b, const V4f& a) {
    return { b / a.x, b / a.y, b / a.z, b / a.w }; }



inline void operator-=(V2f& a, const V2f& b) { a = a - b; }
inline void operator+=(V2f& a, const V2f& b) { a = a + b; }

inline V2f operator/(const V2f& a, F32 b) {
    return { a.x / b, a.y / b }; }

inline V2f operator/(F32 b, const V2f& a) {
    return { b / a.x, b / a.y }; }

inline V2f operator/(const V2f& a, const V2f& b) {
    return { a.x / b.x, a.y / b.y }; }

inline V2f operator*(const V2f& a, F32 b) {
    return { a.x * b, a.y * b }; }

inline V2f operator*(F32 b, const V2f& a) {
    return { a.x * b, a.y * b }; }

inline V2f operator*(const V2f& a, const V2f& b) {
    return { a.x * b.x, a.y * b.y }; }







inline void operator-=(V2i& a, const V2i& b) { a = a - b; }
inline void operator+=(V2i& a, const V2i& b) { a = a + b; }

inline V2i operator/(const V2i& a, S32 b) {
    return { a.x / b, a.y / b }; }

inline V2i operator/(S32 b, const V2i& a) {
    return { b / a.x, b / a.y }; }

inline V2i operator*(const V2i& a, S32 b) {
    return { a.x * b, a.y * b }; }

inline V2i operator*(S32 b, const V2i& a) {
    return { a.x * b, a.y * b }; }











// TODO: remove pass py ref shit
struct Mat4f {

    // COLUMN major
    F32 elems[16] = { 0 };

    Mat4f() { }

    Mat4f(F32 diag) {
        this->elems[0] = diag;
        this->elems[5] = diag;
        this->elems[10] = diag;
        this->elems[15] = diag;
    }

    // CLEANUP: this is bad, please fix it
    Mat4f(F32 _elems[16]) {
        for(int i = 0; i < 16; i++) {
            this->elems[i] = _elems[i];
        }
    }

    const F32& operator[](int i) const {
        DEBUG_ASSERT(i < 16 && i > -1);
        return this->elems[i]; }

    const F32& operator[](U32 i) const {
        DEBUG_ASSERT(i < 16 && i > -1);
        return this->elems[i]; }

    F32& operator[](int i) {
        DEBUG_ASSERT(i < 16 && i > -1);
        return this->elems[i]; }

    F32& operator[](U32 i) {
        DEBUG_ASSERT(i < 16 && i > -1);
        return this->elems[i]; }

    inline F32& at(int c, int r){
        DEBUG_ASSERT(r < 4 && r > -1 && c < 4 && c > -1);
        return this->elems[c*4+r];
    }
};



// :(
V4f operator*(const V4f& lhs, const Mat4f& rhs);
Mat4f operator*(const Mat4f& lhs, const Mat4f& rhs);

bool matrixInverse(Mat4f m, Mat4f& invOut);
void matrixOrtho(float l, float r, float b, float t, float n, float f, Mat4f& out);
void matrixPerspective(float fovY, float aspect, float near, float far, Mat4f& out);
void matrixZRotation(F32 deg, Mat4f& out);
void matrixTranslation(V2f t, Mat4f& out);
void matrixTranslation(V2f t, F32 z, Mat4f& out);
void matrixScale(V2f scale, Mat4f& out);
void matrixScale(V2f scale, F32 z, Mat4f& out);
void matrixTransform(V2f translation, F32 z, F32 deg, V2f scale, Mat4f& out);





#endif