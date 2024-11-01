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

#include <cmath>

#ifdef _MSC_VER
  #ifdef min
    #undef min
  #endif

  #ifdef max
    #undef max
  #endif

  #define NOMINMAX
#endif

namespace RoxMath
{

inline float max(float a,float b) { return fmaxf(a,b); }
inline float min(float a,float b) { return fminf(a,b); }
inline float clamp(float value,float from,float to) { return max(from,min(value,to)); }
inline float lerp(float from,float to,float t) { return from*(1.0f-t)+to*t; }

inline float fade(float time,float time_max,float start_period,float end_period)
{
    const float eps=0.001f;
    if(start_period>eps)
    {
        if(time<0.0f)
            return 0.0f;

        if(time<start_period)
            return time/start_period;
    }

    if(end_period>eps)
    {
        if(time>time_max)
            return 0.0f;

        if(time>time_max-end_period)
            return (time_max-time)/end_period;
    }

    return 1.0f;
}

}
