#include "utils.h"
#include <math.h>



F32 lerp(F32 a, F32 b, F32 t) {
    return a + ((b-a) * t);
}

V4f v4f_lerp(V4f a, V4f b, F32 t) {
    return V4f(
        lerp(a.x, b.x, t),
        lerp(a.y, b.y, t),
        lerp(a.z, b.z, t),
        lerp(a.w, b.w, t)
    );
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










void matrixTranspose(Mat4f m, Mat4f& out) {
    // TODO: this
    assert("this bitch not implemented" == "");
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

void matrixTranslation(V2f t, Mat4f& out) {
    out = Mat4f(1.0f);
    out.at(3, 0) = t.x;
    out.at(3, 1) = t.y;
}

void matrixTranslation(V2f t, F32 z, Mat4f& out) {
    out = Mat4f(1.0f);
    out.at(3, 0) = t.x;
    out.at(3, 1) = t.y;
    out.at(3, 2) = z;
}

void matrixScale(V2f scale, Mat4f& out) {
    out = Mat4f(1.0f);
    out.at(0, 0) = scale.x;
    out.at(1, 1) = scale.y;
}

void matrixScale(V2f scale, F32 z, Mat4f& out) {
    out = Mat4f(1.0f);
    out.at(0, 0) = scale.x;
    out.at(1, 1) = scale.y;
    out.at(2, 2) = z;
}

void matrixTransform(V2f translation, F32 z, F32 deg, V2f scale, Mat4f& out) {
    out = Mat4f(1.0f);
    Mat4f temp;

    matrixTranslation(translation, z, out);
    matrixScale(scale, temp);
    out = temp * out;
    matrixZRotation(deg, temp);
    out = temp * out;
}