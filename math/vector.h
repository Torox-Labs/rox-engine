//nya-engine (C) nyan.developer@gmail.com released under the MIT license (see LICENSE)

#pragma once

#include "angle.h"
#include "scalar.h"

namespace nya_math
{

struct vec2
{
    float x,y;

    vec2(): x(0),y(0) {}
    vec2(float x,float y) { this->x=x,this->y=y; }
    explicit vec2(const float *v) { x=v[0],y=v[1]; }

    vec2 &set(float x,float y) { this->x=x,this->y=y; return *this; }

    vec2 operator + (const vec2 &v) const { return vec2(x+v.x,y+v.y); }
    vec2 operator - (const vec2 &v) const { return vec2(x-v.x,y-v.y); }
    vec2 operator * (const vec2 &v) const { return vec2(x*v.x,y*v.y); }
    vec2 operator * (const float a) const { return vec2(x*a,y*a); }
    vec2 operator / (const vec2 &v) const { return vec2(x/v.x,y/v.y); }
    vec2 operator / (const float a) const { return vec2(x/a,y/a); }

    vec2 operator - () const { return vec2(-x,-y); }

    vec2 operator *= (const vec2 &v) { x*=v.x; y*=v.y; return *this; }
    vec2 operator *= (const float a) { x*=a; y*=a; return *this; }
    vec2 operator /= (const float a) { x/=a; y/=a; return *this; }

    vec2 operator += (const vec2 &v) { x+=v.x; y+=v.y; return *this; }
    vec2 operator -= (const vec2 &v) { x-=v.x; y-=v.y; return *this; }

    float length() const { return sqrtf(length_sq()); }
    float length_sq() const { return dot(*this); }

    float dot(const vec2 &v) const { return dot(*this,v); }
    vec2 &normalize() { return *this=normalize(*this); }
    vec2 &abs() { x=fabsf(x); y=fabsf(y); return *this; }
    vec2 &clamp(const vec2 &from,const vec2 &to) { return *this=clamp(*this,from,to); }
    vec2 reflect(const vec2 &n) const { return reflect(*this,n); }
    angle_rad angle(const vec2 &v) const { return angle(*this,v); }
    vec2 &rotate(angle_rad a) { return *this=rotate(*this,a); }

    angle_rad get_angle() const { return (x==0.0f && y==0.0f)?0.0f:atan2(x,y); }

    static vec2 max(const vec2 &a,const vec2 &b) { return vec2(nya_math::max(a.x,b.x),nya_math::max(a.y,b.y)); }
    static vec2 min(const vec2 &a,const vec2 &b) { return vec2(nya_math::min(a.x,b.x),nya_math::min(a.y,b.y)); }

    static float dot(const vec2 &a,const vec2 &b) { return a.x*b.x+a.y*b.y; }
    static vec2 normalize(const vec2 &v) { float len=v.length(); return len<1.0e-6f? vec2(1.0f,0.0f): v*(1.0f/len); }
    static vec2 abs(const vec2 &v) { return vec2(fabsf(v.x),fabsf(v.y)); }
    static vec2 lerp(const vec2 &from,const vec2 &to,float t) { return from+(to-from)*t; }
    static vec2 clamp(const vec2 &v,const vec2 &from,const vec2 &to) { return min(max(v,from),to); }
    static vec2 reflect(const vec2 &v,const vec2 &n) { const float d=dot(v,n); return v-n*(d+d); }
    static angle_rad angle(const vec2 &a,const vec2 &b) { return acos(dot(normalize(a),normalize(b))); }
    static vec2 rotate(const vec2 &v,angle_rad a) { const float c=cos(a),s=sin(a); return vec2(c*v.x-s*v.y,s*v.x+c*v.y); }

    static const vec2 &right() { static vec2 v(1.0f,0.0f); return v; }
    static const vec2 &up() { static vec2 v(0.0f,1.0f); return v; }

    static const vec2 &zero() { static vec2 v(0.0f,0.0f); return v; }
    static const vec2 &one() { static vec2 v(1.0f,1.0f); return v; }
};

inline vec2 operator * (float a,const vec2 &v) { return vec2(a*v.x,a*v.y); }
inline vec2 operator / (float a,const vec2 &v) { return vec2(a/v.x,a/v.y); }

struct vec3
{
    float x,y,z;

    vec3(): x(0),y(0),z(0) {}
    vec3(float x,float y,float z) { this->x=x,this->y=y,this->z=z; }
    vec3(const nya_math::vec2 &v,float z) { this->x=v.x,this->y=v.y,this->z=z; }
    explicit vec3(const float *v) { x=v[0],y=v[1],z=v[2]; }

    vec3 &set(float x,float y,float z) { this->x=x,this->y=y,this->z=z; return *this; }
    vec3 &set(const nya_math::vec2 &v,float z) { this->x=v.x,this->y=v.y,this->z=z; return *this; }

    vec3 operator + (const vec3 &v) const { return vec3(x+v.x,y+v.y,z+v.z); }
    vec3 operator - (const vec3 &v) const { return vec3(x-v.x,y-v.y,z-v.z); }
    vec3 operator * (const vec3 &v) const { return vec3(x*v.x,y*v.y,z*v.z); }
    vec3 operator * (const float a) const { return vec3(x*a,y*a,z*a); }
    vec3 operator / (const vec3 &v) const { return vec3(x/v.x,y/v.y,z/v.z); }
    vec3 operator / (const float a) const { return vec3(x/a,y/a,z/a); }

    vec3 operator - () const { return vec3(-x,-y,-z); }

    vec3 operator *= (const vec3 &v) { x*=v.x; y*=v.y; z*=v.z; return *this; }
    vec3 operator *= (const float a) { x*=a; y*=a; z*=a; return *this; }
    vec3 operator /= (const float a) { x/=a; y/=a; z/=a; return *this; }

    vec3 operator += (const vec3 &v) { x+=v.x; y+=v.y; z+=v.z; return *this; }
    vec3 operator -= (const vec3 &v) { x-=v.x; y-=v.y; z-=v.z; return *this; }

    float length() const { return sqrtf(length_sq()); }
    float length_sq() const { return dot(*this); }

    float dot(const vec3 &v) const { return dot(*this,v); }
    vec3 cross(const vec3 &v) const { return cross(*this,v); }
    vec3 &normalize() { return *this=normalize(*this); }
    vec3 &abs() { x=fabsf(x); y=fabsf(y); z=fabsf(z); return *this; }
    vec3 &clamp(const vec3 &from,const vec3 &to) { return *this=clamp(*this,from,to); }
    vec3 reflect(const vec3 &n) const { return reflect(*this,n); }
    angle_rad angle(const vec3 &v) const { return angle(*this,v); }

    const vec2 &xy() const { return *(vec2*)&x; }
    vec2 &xy() { return *(vec2*)&x; }

    angle_rad get_yaw() const { return (x==0.0f && z==0.0f)?0.0f:atan2(-x,-z); }
    angle_rad get_pitch() const
    {
        const float l=length(), eps=1.0e-6f;
        if(l>eps)
            return -asin(y/l);
        if(y>eps)
            return -asin(1.0f);
        if(y<-eps)
            return asin(1.0f);
        return angle_rad();
    }

    static vec3 max(const vec3 &a,const vec3 &b) { return vec3(nya_math::max(a.x,b.x),nya_math::max(a.y,b.y),nya_math::max(a.z,b.z)); }
    static vec3 min(const vec3 &a,const vec3 &b) { return vec3(nya_math::min(a.x,b.x),nya_math::min(a.y,b.y),nya_math::min(a.z,b.z)); }

    static float dot(const vec3 &a,const vec3 &b) { return a.x*b.x+a.y*b.y+a.z*b.z; }
    static vec3 cross(const vec3 &a,const vec3 &b) { return vec3(a.y*b.z - a.z*b.y, a.z*b.x - a.x*b.z, a.x*b.y - a.y*b.x); }
    static vec3 normalize(const vec3 &v) { float len=v.length(); return len<1.0e-6f? vec3(1.0f,0.0f,0.0f): v*(1.0f/len); }
    static vec3 abs(const vec3 &v) { return vec3(fabsf(v.x),fabsf(v.y),fabsf(v.z)); }
    static vec3 lerp(const vec3 &from,const vec3 &to,float t) { return from+(to-from)*t; }
    static vec3 clamp(const vec3 &v,const vec3 &from,const vec3 &to) { return min(max(v,from),to); }
    static vec3 reflect(const vec3 &v,const vec3 &n) { const float d=dot(v,n); return v-n*(d+d); }
    static angle_rad angle(const vec3 &a,const vec3 &b) { return acos(dot(normalize(a),normalize(b))); }

    static const vec3 &forward() { static vec3 v(0.0f,0.0f,-1.0f); return v; }
    static const vec3 &right() { static vec3 v(1.0f,0.0f,0.0f); return v; }
    static const vec3 &up() { static vec3 v(0.0f,1.0f,0.0f); return v; }

    static const vec3 &zero() { static vec3 v(0.0f,0.0f,0.0f); return v; }
    static const vec3 &one() { static vec3 v(1.0f,1.0f,1.0f); return v; }
};

inline vec3 operator * (float a,const vec3 &v) { return vec3(a*v.x,a*v.y,a*v.z); }
inline vec3 operator / (float a,const vec3 &v) { return vec3(a/v.x,a/v.y,a/v.z); }

struct vec4
{
    float x,y,z,w;

    vec4(): x(0),y(0),z(0),w(0) {}
    vec4(float x,float y,float z,float w) { this->x=x,this->y=y,this->z=z,this->w=w; }
    vec4(const vec3 &v,float w) { this->x=v.x,this->y=v.y,this->z=v.z,this->w=w; }
    explicit vec4(const float *v) { x=v[0],y=v[1],z=v[2],w=v[3]; }

    vec4 &set(float x,float y,float z,float w) { this->x=x,this->y=y,this->z=z,this->w=w; return *this; }
    vec4 &set(const nya_math::vec3 &v,float w) { this->x=v.x,this->y=v.y,this->z=v.z,this->w=w; return *this; }

    vec4 operator + (const vec4 &v) const { return vec4(x+v.x,y+v.y,z+v.z,w+v.w); }

    vec4 operator - (const vec4 &v) const { return vec4(x-v.x,y-v.y,z-v.z,w-v.w); }
    vec4 operator * (const vec4 &v) const { return vec4(x*v.x,y*v.y,z*v.z,w*v.w); }
    vec4 operator * (const float a) const { return vec4(x*a,y*a,z*a,w*a); }
    vec4 operator / (const vec4 &v) const { return vec4(x/v.x,y/v.y,z/v.z,w/v.w); }
    vec4 operator / (const float a) const { return vec4(x/a,y/a,z/a,w/a); }

    vec4 operator - () const { return vec4(-x,-y,-z,-w); }

    vec4 operator *= (const vec4 &v) { x*=v.x; y*=v.y; z*=v.z; w*=v.w; return *this; }
    vec4 operator *= (const float a) { x*=a; y*=a; z*=a; w*=a; return *this; }
    vec4 operator /= (const float a) { x/=a; y/=a; z/=a; w/=a; return *this; }

    vec4 operator += (const vec4 &v) { x+=v.x; y+=v.y; z+=v.z; w+=v.w; return *this; }
    vec4 operator -= (const vec4 &v) { x-=v.x; y-=v.y; z-=v.z; w+=v.w; return *this; }

    float length() const { return sqrtf(length_sq()); }
    float length_sq() const { return dot(*this); }

    float dot(const vec4 &v) const { return dot(*this,v); }
    vec4 &normalize() { return *this=normalize(*this); }
    vec4 &abs() { x=fabsf(x); y=fabsf(y); z=fabsf(z); w=fabsf(w); return *this; }

    const vec3 &xyz() const { return *(vec3*)&x; }
    vec3 &xyz() { return *(vec3*)&x; }
    const vec2 &xy() const { return *(vec2*)&x; }
    vec2 &xy() { return *(vec2*)&x; }
    const vec2 &zw() const { return *(vec2*)&z; }
    vec2 &zw() { return *(vec2*)&z; }

    static vec4 max(const vec4 &a,const vec4 &b) { return vec4(nya_math::max(a.x,b.x),nya_math::max(a.y,b.y),
                                                               nya_math::max(a.z,b.z),nya_math::max(a.w,b.w)); }
    static vec4 min(const vec4 &a,const vec4 &b) { return vec4(nya_math::min(a.x,b.x),nya_math::min(a.y,b.y),
                                                               nya_math::min(a.z,b.z),nya_math::min(a.w,b.w)); }

    static float dot(const vec4 &a,const vec4 &b) { return a.x*b.x+a.y*b.y+a.z*b.z+a.w*b.w; }
    static vec4 normalize(const vec4 &v) { float len=v.length(); return len<1.0e-6f? vec4(1.0f,0.0f,0.0f,0.0f): v*(1.0f/len); }
    static vec4 abs(const vec4 &v) { return vec4(fabsf(v.x),fabsf(v.y),fabsf(v.z),fabsf(v.w)); }
    static vec4 lerp(const vec4 &from,const vec4 &to,float t) { return from+(to-from)*t; }
    static vec4 clamp(const vec4 &v,const vec4 &from,const vec4 &to) { return min(max(v,from),to); }

    static const vec4 &zero() { static vec4 v(0.0f,0.0f,0.0f,0.0f); return v; }
    static const vec4 &one() { static vec4 v(1.0f,1.0f,1.0f,1.0f); return v; }
};

inline vec4 operator * (float a,const vec4 &v) { return vec4(a*v.x,a*v.y,a*v.z,a*v.w); }
inline vec4 operator / (float a,const vec4 &v) { return vec4(a/v.x,a/v.y,a/v.z,a/v.w); }

}

