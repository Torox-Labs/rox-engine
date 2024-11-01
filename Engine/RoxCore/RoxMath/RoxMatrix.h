// Updated By the ROX_ENGINE
// Copyright (C) 2024 Torox Project
// Portions Copyright (C) 2013 nyan.developer@gmail.com (nya-engine)
//
// This file was modified by the Torox Project.
// 
// This file incorporates code from the nya-engine project, which is licensed under the MIT License.
// See the LICENSE-MIT file in the root directory for more information.
//
// This file is also part of the Rox-engine, which is licensed under a dual-license system:
// 1. Free Use License (for non-commercial and commercial use under specific conditions)
// 2. Commercial License (for use on proprietary platforms)
// See the LICENSE file in the root directory for the full Rox-engine license terms.

#pragma once

#include "RoxAngle.h"

namespace RoxMath
{

struct Vector3;
struct Vector4;
struct Quaternion;

#ifdef __GNUC__
    #define align16 __attribute__((aligned(16)))
#else
    #define align16 __declspec(align(16))
#endif

struct align16 Matrix4
{
    float m[4][4];

    Matrix4 &identity();
    Matrix4 &translate(float x,float y,float z);
    Matrix4 &translate(const Vector3 &v);
    Matrix4 &rotate(AngleDeg angle,float x,float y,float z);
    Matrix4 &rotate(AngleDeg angle,const Vector3 &v);
    Matrix4 &rotate(const Quaternion &q);
    Matrix4 &scale(float sx,float sy,float sz);
    Matrix4 &scale(const Vector3 &v);
    Matrix4 &scale(float s) { return scale(s,s,s); }

    Matrix4 &set(const Vector3 &p,const Quaternion &r);
    Matrix4 &set(const Vector3 &p,const Quaternion &r,const Vector3 &s);

    Matrix4 &perspective(AngleDeg fov,float aspect,float near,float far);
    Matrix4 &frustrum(float left,float right,float bottom,float top,float near,float far);
    Matrix4 &ortho(float left,float right,float bottom,float top,float near,float far);

    Matrix4 &invert();
    Matrix4 &transpose();

    Quaternion get_rot() const;
    Vector3 get_pos() const;

    Matrix4 operator * (const Matrix4 &mat) const;

    const float * operator [] (int i) const { return m[i]; }
    float * operator [] (int i) { return m[i]; }

    Matrix4() { identity(); }
    Matrix4(const float (&m)[4][4],bool transpose=false);
    Matrix4(const float (&m)[4][3]);
    Matrix4(const float (&m)[3][4]); //transposed
    Matrix4(const Quaternion &q);
    Matrix4(const Vector3 &p, const Quaternion &r) { set(p,r); }
    Matrix4(const Vector3 &p, const Quaternion &r, const Vector3 &s) { set(p,r,s); }
};

Vector3 operator * (const Matrix4 &m,const Vector3 &v);
Vector3 operator * (const Vector3 &v,const Matrix4 &m);
Vector4 operator * (const Vector4 &v,const Matrix4 &m);
Vector4 operator * (const Matrix4 &m,const Vector4 &v);

}
