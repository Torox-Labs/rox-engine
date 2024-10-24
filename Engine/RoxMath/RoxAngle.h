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

#include "RoxConstants.h"
#include "RoxScalar.h"
#include <algorithm>

namespace RoxMath
{

template<typename t> struct Angle
{
    float value;

    Angle(): value(0.0f) {}
    Angle(float a): value(a) {}

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

    t &clamp(const t &from,const t &to) { *this= RoxMath::clamp(value,from.value,to.value); return *(t*)this; }
    static t clamp(const t &a,const t &from,const t &to) { return RoxMath::clamp(a.value,from.value,to.value); }
};

template<typename t> t operator + (float a,const Angle<t> &b) { return t(a+b.value); }
template<typename t> t operator - (float a,const Angle<t> &b) { return t(a-b.value); }
template<typename t> t operator * (float a,const Angle<t> &b) { return t(a*b.value); }
template<typename t> t operator / (float a,const Angle<t> &b) { return t(a/b.value); }

struct AngleDeg;

struct AngleRad: public Angle<AngleRad>
{
    AngleRad(): Angle(0.0f) {}
    AngleRad(float a): Angle(a) {}
    AngleRad(const AngleDeg& a);

    void setDeg(float a) { value=a/(180.0f/Constants::pi); }
    void setRad(float a) { value=a; }

    float getDeg() const { return value/(Constants::pi/180.0f); }
    float getRad() const { return value; }

    AngleRad normalize() { *this=normalize(*this).value; return *this; }
    bool is_between(AngleRad from,AngleRad to) const { return is_between(*this,from,to); }

    static AngleRad normalize(const AngleRad &a)
    {
        const float r=fmodf(a.value+Constants::pi,Constants::pi*2.0f);
        return r<0.0f ? r+Constants::pi : r-Constants::pi;
    }

    static bool is_between(AngleRad angle,AngleRad from,AngleRad to)
    {
        const AngleRad diff=normalize(to-from);
        if(diff>=Constants::pi)
            std::swap(from,to);

        if(from<=to)
            return angle>=from && angle<=to;

        return angle>=from || angle<=to;
    }
};

struct AngleDeg: public Angle<AngleDeg>
{
    AngleDeg(): Angle(0.0f) {}
    AngleDeg(float a): Angle(a) {}
    AngleDeg(const AngleRad& a);

    void setDeg(float a) { value=a; }
    void setRad(float a) { value=a/(Constants::pi/180.0f); }

    float getDeg() const { return value; }
    float getRad() const { return value/(180.0f/Constants::pi); }

    AngleDeg normalize() { *this=normalize(*this).value; return *this; }
    bool is_between(AngleDeg from,AngleDeg to) const { return is_between(*this,from,to); }

    static AngleDeg normalize(const AngleDeg &a)
    {
        const float r=fmodf(a.value+180.0f,360.0f);
        return r<0.0f ? r+180.0f : r-180.0f;
    }

    static bool is_between(AngleDeg angle,AngleDeg from,AngleDeg to)
    {
        const AngleDeg diff=normalize(to-from);
        if(diff>=180.0f)
            std::swap(from,to);

        if(from<=to)
            return angle>=from && angle<=to;

        return angle>=from || angle<=to;
    }
};

template<typename t> float sin(const t &a) { return sinf(a.getRad()); }
template<typename t> float cos(const t &a) { return cosf(a.getRad()); }
template<typename t> float tan(const t &a) { return tanf(a.getRad()); }

inline AngleRad asin(float a) { return asinf(a); }
inline AngleRad acos(float a) { return acosf(a); }
inline AngleRad atan(float a) { return atanf(a); }
inline AngleRad atan2(float a,float b) { return atan2f(a,b); }

inline AngleRad::AngleRad(const AngleDeg& a) { value=a.getRad(); }
inline AngleDeg::AngleDeg(const AngleRad& a) { value=a.getDeg(); }

}
