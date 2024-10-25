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

#include "RoxVector.h"

namespace RoxMath
{

struct Matrix4;

struct Quaternion
{
    Vector3 v;
    float w;

    Quaternion(): w(1.0f) {}

    Quaternion(float x,float y,float z,float w)
    {
        v.x=x; v.y=y;
        v.z=z; this->w=w;
    }

    Quaternion(const Matrix4 &m);

    Quaternion(AngleRad pitch,AngleRad yaw,AngleRad roll);

    Quaternion(const Vector3 &axis,AngleRad angle);

    Quaternion(const Vector3 &from,const Vector3 &to);

    explicit Quaternion(const float *q) { v.x=q[0]; v.y=q[1]; v.z=q[2]; w=q[3]; }

    Quaternion operator - () const { return Quaternion(-v.x,-v.y,-v.z,-w); }

    Quaternion operator * (const Quaternion &q) const;
    Quaternion operator *= (const Quaternion &q) { *this=*this*q; return *this; }

    Vector3 getEuler() const; //pitch,yaw,roll in radians

    Quaternion &normalize();

    Quaternion &invert() { v= -v; return *this; }

    Quaternion &applyWeight(float weight);

    Vector3 rotate(const Vector3 &v) const;
    Vector3 rotateInv(const Vector3 &v) const;

    static Quaternion slerp(const Quaternion &from,const Quaternion &to,float t);
    static Quaternion nlerp(const Quaternion &from,const Quaternion &to,float t);
    static Quaternion invert(const Quaternion &q) { Quaternion r=q; r.invert(); return r; }
};

}
