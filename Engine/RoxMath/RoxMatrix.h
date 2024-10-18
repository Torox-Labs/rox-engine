//nya-engine (C) nyan.developer@gmail.com released under the MIT license (see LICENSE)

#pragma once

#include "angle.h"

namespace nya_math
{

struct vec3;
struct vec4;
struct quat;

#ifdef __GNUC__
    #define align16 __attribute__((aligned(16)))
#else
    #define align16 __declspec(align(16))
#endif

struct align16 mat4
{
    float m[4][4];

    mat4 &identity();
    mat4 &translate(float x,float y,float z);
    mat4 &translate(const vec3 &v);
    mat4 &rotate(angle_deg angle,float x,float y,float z);
    mat4 &rotate(angle_deg angle,const vec3 &v);
    mat4 &rotate(const quat &q);
    mat4 &scale(float sx,float sy,float sz);
    mat4 &scale(const vec3 &v);
    mat4 &scale(float s) { return scale(s,s,s); }

    mat4 &set(const vec3 &p,const quat &r);
    mat4 &set(const vec3 &p,const quat &r,const vec3 &s);

    mat4 &perspective(angle_deg fov,float aspect,float near,float far);
    mat4 &frustrum(float left,float right,float bottom,float top,float near,float far);
    mat4 &ortho(float left,float right,float bottom,float top,float near,float far);

    mat4 &invert();
    mat4 &transpose();

    quat get_rot() const;
    vec3 get_pos() const;

    mat4 operator * (const mat4 &mat) const;

    const float * operator [] (int i) const { return m[i]; }
    float * operator [] (int i) { return m[i]; }

    mat4() { identity(); }
    mat4(const float (&m)[4][4],bool transpose=false);
    mat4(const float (&m)[4][3]);
    mat4(const float (&m)[3][4]); //transposed
    mat4(const quat &q);
    mat4(const vec3 &p, const quat &r) { set(p,r); }
    mat4(const vec3 &p, const quat &r, const vec3 &s) { set(p,r,s); }
};

vec3 operator * (const mat4 &m,const vec3 &v);
vec3 operator * (const vec3 &v,const mat4 &m);
vec4 operator * (const vec4 &v,const mat4 &m);
vec4 operator * (const mat4 &m,const vec4 &v);

}
