//nya-engine (C) nyan.developer@gmail.com released under the MIT license (see LICENSE)

#pragma once

#include "constants.h"
#include "scalar.h"
#include <algorithm>
#include <math.h>

namespace nya_math
{

template<typename t> struct angle
{
    float value;

    angle(): value(0.0f) {}
    angle(float a): value(a) {}

    t operator + (const t &a) const { return t(value+a.value); }
    t operator - (const t &a) const { return t(value-a.value); }
    t operator * (const t &a) const { return t(value*a.value); }
    t operator / (const t &a) const { return t(value/a.value); }

    t operator - () const { return t(-value); }

    t operator += (const t &a) { value+=a.value; return *(t*)this; }
    t operator -= (const t &a) { value-=a.value; return *(t*)this; }
    t operator *= (const t &a) { value*=a.value; return *(t*)this; }
    t operator /= (const t &a) { value/=a.value; return *(t*)this; }

    bool operator < (const t &a) const { return value < a.value; }
    bool operator > (const t &a) const { return value > a.value; }
    bool operator <= (const t &a) const { return value <= a.value; }
    bool operator >= (const t &a) const { return value >= a.value; }

    t &clamp(const t &from,const t &to) { *this=nya_math::clamp(value,from.value,to.value); return *(t*)this; }
    static t clamp(const t &a,const t &from,const t &to) { return nya_math::clamp(a.value,from.value,to.value); }
};

template<typename t> t operator + (float a,const angle<t> &b) { return t(a+b.value); }
template<typename t> t operator - (float a,const angle<t> &b) { return t(a-b.value); }
template<typename t> t operator * (float a,const angle<t> &b) { return t(a*b.value); }
template<typename t> t operator / (float a,const angle<t> &b) { return t(a/b.value); }

struct angle_deg;

struct angle_rad: public angle<angle_rad>
{
    angle_rad(): angle(0.0f) {}
    angle_rad(float a): angle(a) {}
    angle_rad(const angle_deg& a);

    void set_deg(float a) { value=a/(180.0f/constants::pi); }
    void set_rad(float a) { value=a; }

    float get_deg() const { return value/(constants::pi/180.0f); }
    float get_rad() const { return value; }

    angle_rad normalize() { *this=normalize(*this).value; return *this; }
    bool is_between(angle_rad from,angle_rad to) const { return is_between(*this,from,to); }

    static angle_rad normalize(const angle_rad &a)
    {
        const float r=fmodf(a.value+constants::pi,constants::pi*2.0f);
        return r<0.0f ? r+constants::pi : r-constants::pi;
    }

    static bool is_between(angle_rad angle,angle_rad from,angle_rad to)
    {
        const angle_rad diff=normalize(to-from);
        if(diff>=constants::pi)
            std::swap(from,to);

        if(from<=to)
            return angle>=from && angle<=to;

        return angle>=from || angle<=to;
    }
};

struct angle_deg: public angle<angle_deg>
{
    angle_deg(): angle(0.0f) {}
    angle_deg(float a): angle(a) {}
    angle_deg(const angle_rad& a);

    void set_deg(float a) { value=a; }
    void set_rad(float a) { value=a/(constants::pi/180.0f); }

    float get_deg() const { return value; }
    float get_rad() const { return value/(180.0f/constants::pi); }

    angle_deg normalize() { *this=normalize(*this).value; return *this; }
    bool is_between(angle_deg from,angle_deg to) const { return is_between(*this,from,to); }

    static angle_deg normalize(const angle_deg &a)
    {
        const float r=fmodf(a.value+180.0f,360.0f);
        return r<0.0f ? r+180.0f : r-180.0f;
    }

    static bool is_between(angle_deg angle,angle_deg from,angle_deg to)
    {
        const angle_deg diff=normalize(to-from);
        if(diff>=180.0f)
            std::swap(from,to);

        if(from<=to)
            return angle>=from && angle<=to;

        return angle>=from || angle<=to;
    }
};

template<typename t> float sin(const t &a) { return sinf(a.get_rad()); }
template<typename t> float cos(const t &a) { return cosf(a.get_rad()); }
template<typename t> float tan(const t &a) { return tanf(a.get_rad()); }

inline angle_rad asin(float a) { return asinf(a); }
inline angle_rad acos(float a) { return acosf(a); }
inline angle_rad atan(float a) { return atanf(a); }
inline angle_rad atan2(float a,float b) { return atan2f(a,b); }

inline angle_rad::angle_rad(const angle_deg& a) { value=a.get_rad(); }
inline angle_deg::angle_deg(const angle_rad& a) { value=a.get_deg(); }

}
