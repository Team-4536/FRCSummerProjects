#pragma once
#include "base/typedefs.h"
#include "base/allocators.h"
#include "base/config.h"

// CLEANUP: remove this include
#include <math.h>


F32 lerp(F32 a, F32 b, F32 t);

constexpr F32 radians(F32 d);
constexpr F32 degrees(F32 r);

// returns nullptr on failed file open, else pointer to buffer
// outsize can be nullptr
U8* loadFileToBuffer(const char* path, bool asText, U64* outSize, BumpAlloc* arena);



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
    V2f(F32 s): x(s), y(s) { }
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

V2f v2f_lerp(V2f a, V2f b, F32 t);

// TODO: v3


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

    inline V4f operator*(const V4f& b) const {
        return { this->x * b.x, this->y * b.y, this->z * b.z, this->w * b.w }; }


    // this really needs to be less awful, but c++ is a terrible language
    F32& operator[](U32 i) {
        switch (i)
        {
        case 0: return this->x; break;
        case 1: return this->y; break;
        case 2: return this->z; break;
        case 3: return this->w; break;
        default: ASSERT(false); break;
        }
    }
};

struct Rect2f {
    V2f start = { 0, 0 };
    V2f end = { 0, 0 };
};


V4f v4f_lerp(V4f a, V4f b, F32 t);

inline void operator-=(V4f& a, const V4f& b) { a = a - b; }
inline void operator+=(V4f& a, const V4f& b) { a = a + b; }
inline void operator*=(V4f& a, const V4f& b) { a = a * b; }

// CLEANUP: V4 operators not done lol
inline V4f operator*(const V4f& a, F32 b) {
    return { a.x * b, a.y * b, a.z * b, a.w * b }; }

inline V4f operator*(F32 b, const V4f& a) {
    return { a.x * b, a.y * b, a.z * b, a.w * b }; }

inline V4f operator/(const V4f& a, F32 b) {
    return { a.x / b, a.y / b, a.z / b, a.w / b }; }

inline V4f operator/(F32 b, const V4f& a) {
    return { b / a.x, b / a.y, b / a.z, b / a.w }; }



F32 v2fAngle(const V2f& a);
F32 v2fDot(const V2f& a, const V2f& b);
V2f v2fNormalized(const V2f& a); // NOTE: is it good practice to const ref a small struct like vectors?

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

inline void operator*=(V2f& a, const V2f& b) { a = a * b; }
inline void operator/=(V2f& a, const V2f& b) { a = a / b; }
inline void operator*=(V2f& a, float s) { a = a * s; }
inline void operator/=(V2f& a, float s) { a = a / s; }


// V2I ////////////////////////////////////////////////////////////////
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


V4f operator*(const V4f& lhs, const Mat4f& rhs);
Mat4f operator*(const Mat4f& lhs, const Mat4f& rhs);

// TODO: return based instead of ref based
bool matrixInverse(Mat4f m, Mat4f& invOut);
void matrixOrtho(float l, float r, float b, float t, float n, float f, Mat4f& out);
void matrixPerspective(float fovY, float aspect, float near, float far, Mat4f& out);

void matrixZRotation(F32 deg, Mat4f& out);
void matrixXRotation(F32 deg, Mat4f& out);
void matrixYRotation(F32 deg, Mat4f& out);
void matrixTranslation(F32 x, F32 y, F32 z, Mat4f& out);
void matrixScale(F32 x, F32 y, F32 z, Mat4f& out);


// S for scale, R for rotation (in degrees), others are translation
struct Transform {
    float x = 0;
    float y = 0;
    float z = 0;

    float rx = 0;
    float ry = 0;
    float rz = 0;

    float sx = 1;
    float sy = 1;
    float sz = 1;
};
Mat4f matrixTransform(Transform t);


#ifdef BASE_IMPL

constexpr F32 radians(F32 d) {
    return d * M_PI/180;
}
constexpr F32 degrees(F32 r) {
    return r * 180/M_PI;
}



F32 lerp(F32 a, F32 b, F32 t) {
    return a + ((b-a) * t);
}

V2f v2f_lerp(V2f a, V2f b, F32 t) {
    return V2f(
        lerp(a.x, b.x, t),
        lerp(a.y, b.y, t)
    );
}

V4f v4f_lerp(V4f a, V4f b, F32 t) {
    return V4f(
        lerp(a.x, b.x, t),
        lerp(a.y, b.y, t),
        lerp(a.z, b.z, t),
        lerp(a.w, b.w, t)
    );
}



F32 v2fAngle(const V2f& a) {
    return degrees(-atan2f(a.y, a.x));
}



F32 v2fDot(const V2f& a, const V2f& b) {
    return a.x * b.x + a.y * b.y;
}
V2f v2fNormalized(const V2f& a) {
    F32 len = a.length();
    if (len == 0) { return V2f(); }
    return a / len;
}

V4f operator*(const V4f& lhs, const Mat4f& rhs) {

    V4f out;
    out.x = ( (lhs.x * rhs[0]) + (lhs.y * rhs[4]) + (lhs.z * rhs[8]) + (lhs.w * rhs[12]) );
    out.y = ( (lhs.x * rhs[1]) + (lhs.y * rhs[5]) + (lhs.z * rhs[9]) + (lhs.w * rhs[13]) );
    out.z = ( (lhs.x * rhs[2]) + (lhs.y * rhs[6]) + (lhs.z * rhs[10]) + (lhs.w * rhs[14]) );
    out.w = ( (lhs.x * rhs[3]) + (lhs.y * rhs[7]) + (lhs.z * rhs[11]) + (lhs.w * rhs[15]) );

    return out;
}

Mat4f operator*(const Mat4f& lhs, const Mat4f& rhs) {

    Mat4f out;
    for(int r = 0; r < 4; r++) {
        for(int c = 0; c < 4; c++) {

            F32 sum0 = (lhs[c*4+0] * rhs[0*4+r]) + (lhs[c*4+1] * rhs[1*4+r]);
            F32 sum1 = (lhs[c*4+2] * rhs[2*4+r]) + (lhs[c*4+3] * rhs[3*4+r]);
            out[c*4+r] = sum0 + sum1;
        }
    }

    return out;
}





// NOTE: I cant remember where i stole this shit from but i definitely did not make it
bool matrixInverse(Mat4f m, Mat4f& invOut)
{
    double inv[16], det;
    int i;

    inv[0] = m[5]  * m[10] * m[15] -
             m[5]  * m[11] * m[14] -
             m[9]  * m[6]  * m[15] +
             m[9]  * m[7]  * m[14] +
             m[13] * m[6]  * m[11] -
             m[13] * m[7]  * m[10];

    inv[4] = -m[4]  * m[10] * m[15] +
              m[4]  * m[11] * m[14] +
              m[8]  * m[6]  * m[15] -
              m[8]  * m[7]  * m[14] -
              m[12] * m[6]  * m[11] +
              m[12] * m[7]  * m[10];

    inv[8] = m[4]  * m[9] * m[15] -
             m[4]  * m[11] * m[13] -
             m[8]  * m[5] * m[15] +
             m[8]  * m[7] * m[13] +
             m[12] * m[5] * m[11] -
             m[12] * m[7] * m[9];

    inv[12] = -m[4]  * m[9] * m[14] +
               m[4]  * m[10] * m[13] +
               m[8]  * m[5] * m[14] -
               m[8]  * m[6] * m[13] -
               m[12] * m[5] * m[10] +
               m[12] * m[6] * m[9];

    inv[1] = -m[1]  * m[10] * m[15] +
              m[1]  * m[11] * m[14] +
              m[9]  * m[2] * m[15] -
              m[9]  * m[3] * m[14] -
              m[13] * m[2] * m[11] +
              m[13] * m[3] * m[10];

    inv[5] = m[0]  * m[10] * m[15] -
             m[0]  * m[11] * m[14] -
             m[8]  * m[2] * m[15] +
             m[8]  * m[3] * m[14] +
             m[12] * m[2] * m[11] -
             m[12] * m[3] * m[10];

    inv[9] = -m[0]  * m[9] * m[15] +
              m[0]  * m[11] * m[13] +
              m[8]  * m[1] * m[15] -
              m[8]  * m[3] * m[13] -
              m[12] * m[1] * m[11] +
              m[12] * m[3] * m[9];

    inv[13] = m[0]  * m[9] * m[14] -
              m[0]  * m[10] * m[13] -
              m[8]  * m[1] * m[14] +
              m[8]  * m[2] * m[13] +
              m[12] * m[1] * m[10] -
              m[12] * m[2] * m[9];

    inv[2] = m[1]  * m[6] * m[15] -
             m[1]  * m[7] * m[14] -
             m[5]  * m[2] * m[15] +
             m[5]  * m[3] * m[14] +
             m[13] * m[2] * m[7] -
             m[13] * m[3] * m[6];

    inv[6] = -m[0]  * m[6] * m[15] +
              m[0]  * m[7] * m[14] +
              m[4]  * m[2] * m[15] -
              m[4]  * m[3] * m[14] -
              m[12] * m[2] * m[7] +
              m[12] * m[3] * m[6];

    inv[10] = m[0]  * m[5] * m[15] -
              m[0]  * m[7] * m[13] -
              m[4]  * m[1] * m[15] +
              m[4]  * m[3] * m[13] +
              m[12] * m[1] * m[7] -
              m[12] * m[3] * m[5];

    inv[14] = -m[0]  * m[5] * m[14] +
               m[0]  * m[6] * m[13] +
               m[4]  * m[1] * m[14] -
               m[4]  * m[2] * m[13] -
               m[12] * m[1] * m[6] +
               m[12] * m[2] * m[5];

    inv[3] = -m[1] * m[6] * m[11] +
              m[1] * m[7] * m[10] +
              m[5] * m[2] * m[11] -
              m[5] * m[3] * m[10] -
              m[9] * m[2] * m[7] +
              m[9] * m[3] * m[6];

    inv[7] = m[0] * m[6] * m[11] -
             m[0] * m[7] * m[10] -
             m[4] * m[2] * m[11] +
             m[4] * m[3] * m[10] +
             m[8] * m[2] * m[7] -
             m[8] * m[3] * m[6];

    inv[11] = -m[0] * m[5] * m[11] +
               m[0] * m[7] * m[9] +
               m[4] * m[1] * m[11] -
               m[4] * m[3] * m[9] -
               m[8] * m[1] * m[7] +
               m[8] * m[3] * m[5];

    inv[15] = m[0] * m[5] * m[10] -
              m[0] * m[6] * m[9] -
              m[4] * m[1] * m[10] +
              m[4] * m[2] * m[9] +
              m[8] * m[1] * m[6] -
              m[8] * m[2] * m[5];

    det = m[0] * inv[0] + m[1] * inv[4] + m[2] * inv[8] + m[3] * inv[12];

    if (det == 0)
        return false;

    det = 1.0 / det;

    for (i = 0; i < 16; i++)
        invOut[i] = inv[i] * det;

    return true;
}



void matrixOrtho(float l, float r, float b, float t, float n, float f, Mat4f& out) {
    out = Mat4f(0.0f);
    out[0]  = 2 / (r - l);
    out[5]  = 2 / (t - b);
    out[10] = -2 / (f - n);
    out[12] = -(r + l) / (r - l);
    out[13] = -(t + b) / (t - b);
    out[14] = -(f + n) / (f - n);
    out[15] = 1;
}

void glhFrustumf2(Mat4f& matrix, float left, float right, float bottom, float top,
                  float znear, float zfar) {
    float temp, temp2, temp3, temp4;
    temp = 2.0 * znear;
    temp2 = right - left;
    temp3 = top - bottom;
    temp4 = zfar - znear;
    matrix[0] = temp / temp2;
    matrix[1] = 0.0;
    matrix[2] = 0.0;
    matrix[3] = 0.0;
    matrix[4] = 0.0;
    matrix[5] = temp / temp3;
    matrix[6] = 0.0;
    matrix[7] = 0.0;
    matrix[8] = (right + left) / temp2;
    matrix[9] = (top + bottom) / temp3;
    matrix[10] = (-zfar - znear) / temp4;
    matrix[11] = -1.0;
    matrix[12] = 0.0;
    matrix[13] = 0.0;
    matrix[14] = (-temp * zfar) / temp4;
    matrix[15] = 0.0;
}

void matrixPerspective(float fovY, float aspect, float near, float far, Mat4f& out) {
    float ymax, xmax;
    ymax = near * tanf(fovY * M_PI / 360.0);
    xmax = ymax * aspect;
    glhFrustumf2(out, -xmax, xmax, -ymax, ymax, near, far);
}

void matrixZRotation(F32 deg, Mat4f& out) {
    F32 c = cosf(deg * (M_PI / 180));
    F32 s = sinf(deg * (M_PI / 180));

    F32 e[16] = {
        c, s, 0, 0,
        -s, c, 0, 0,
        0, 0, 1, 0,
        0, 0, 0, 1
    };

    out = Mat4f(e);
}

void matrixXRotation(F32 deg, Mat4f& out) {
    F32 c = cosf(deg * (M_PI / 180));
    F32 s = sinf(deg * (M_PI / 180));

    F32 e[16] = {
        1, 0, 0, 0,
        0, c, s, 0,
        0, -s, c, 0,
        0, 0, 0, 1
    };
    out = Mat4f(e);
}

void matrixYRotation(F32 deg, Mat4f& out) {
    F32 c = cosf(deg * (M_PI / 180));
    F32 s = sinf(deg * (M_PI / 180));

    F32 e[16] = {
        c, 0, -s, 0,
        0, 1, 0, 0,
        s, 0, c, 0,
        0, 0, 0, 1
    };
    out = Mat4f(e);
}


void matrixTranslation(F32 x, F32 y, F32 z, Mat4f& out) {
    out = Mat4f(1.0f);
    out.at(3, 0) = x;
    out.at(3, 1) = y;
    out.at(3, 2) = z;
}

void matrixScale(F32 x, F32 y, F32 z, Mat4f& out) {
    out = Mat4f(1.0f);
    out.at(0, 0) = x;
    out.at(1, 1) = y;
    out.at(2, 2) = z;
}


Mat4f matrixTransform(Transform t) {
    Mat4f temp = Mat4f();
    Mat4f out = Mat4f();

    matrixTranslation(t.x, t.y, t.z, out);
    matrixYRotation(t.ry, temp);
    out = temp * out;
    matrixXRotation(t.rx, temp);
    out = temp * out;
    matrixZRotation(t.rz, temp);
    out = temp * out;
    matrixScale(t.sx, t.sy, t.sz, temp);
    out = temp * out;

    return out;
}

#endif