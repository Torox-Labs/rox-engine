//nya-engine (C) nyan.developer@gmail.com released under the MIT license (see LICENSE)

#pragma once

#include "vector.h"

namespace nya_math
{

struct mat4;

struct quat
{
    vec3 v;
    float w;

    quat(): w(1.0f) {}

    quat(float x,float y,float z,float w)
    {
        v.x=x; v.y=y;
        v.z=z; this->w=w;
    }

    quat(const mat4 &m);

    quat(angle_rad pitch,angle_rad yaw,angle_rad roll);

    quat(const vec3 &axis,angle_rad angle);

    quat(const vec3 &from,const vec3 &to);

    explicit quat(const float *q) { v.x=q[0]; v.y=q[1]; v.z=q[2]; w=q[3]; }

    quat operator - () const { return quat(-v.x,-v.y,-v.z,-w); }

    quat operator * (const quat &q) const;
    quat operator *= (const quat &q) { *this=*this*q; return *this; }

    vec3 get_euler() const; //pitch,yaw,roll in radians

    quat &normalize();

    quat &invert() { v= -v; return *this; }

    quat &apply_weight(float weight);

    vec3 rotate(const vec3 &v) const;
    vec3 rotate_inv(const vec3 &v) const;

    static quat slerp(const quat &from,const quat &to,float t);
    static quat nlerp(const quat &from,const quat &to,float t);
    static quat invert(const quat &q) { quat r=q; r.invert(); return r; }
};

}
