//nya-engine (C) nyan.developer@gmail.com released under the MIT license (see LICENSE)

#pragma once

#include <math.h>

#ifdef _MSC_VER
  #ifdef min
    #undef min
  #endif

  #ifdef max
    #undef max
  #endif

  #define NOMINMAX
#endif

namespace nya_math
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
