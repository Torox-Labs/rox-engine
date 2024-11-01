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

#if defined(__arm__) ||  defined(__ARM_NEON__)  ||  defined(__ARM_NEON) || defined(_M_ARM)
    #define SIMD_NEON
#endif

#ifdef SIMD_NEON
    #include <arm_neon.h>
#else
    #include <xmmintrin.h>
#endif

#ifdef __GNUC__
    #define align16 __attribute__((aligned(16)))
#else
    #define align16 __declspec(align(16))
#endif

namespace RoxMath
{

struct align16 SimdVec4
{
#ifdef SIMD_NEON
    float32x4_t xmm;
#else
    __m128 xmm;
#endif

    SimdVec4()
    {
#ifdef SIMD_NEON
        xmm=vdupq_n_f32(0.0f);
#else
        xmm=_mm_set1_ps(0.0f);
#endif
    }

#ifdef SIMD_NEON
    SimdVec4(const float32x4_t xmm): xmm(xmm) {}
#else
    SimdVec4(const __m128 xmm): xmm(xmm) {}
#endif

    SimdVec4(const float *v)
    {
#ifdef SIMD_NEON
        xmm=vld1q_f32(v);
#else
        xmm=_mm_loadu_ps(v);
#endif
    }

    SimdVec4(float v)
    {
#ifdef SIMD_NEON
        xmm=vdupq_n_f32(v);
#else
        xmm=_mm_set1_ps(v);
#endif
    }

    void set(const float *v)
    {
#ifdef SIMD_NEON
        xmm=vld1q_f32(v);
#else
        xmm=_mm_loadu_ps(v);
#endif
    }

    void get(float *v) const
    {
#ifdef SIMD_NEON
        vst1q_f32(v,xmm);
#else
        _mm_storeu_ps(v,xmm);
#endif
    }

    SimdVec4 operator * (const SimdVec4 &v) const
    {
#ifdef SIMD_NEON
        return SimdVec4(vmulq_f32(xmm,v.xmm));
#else
        return SimdVec4(_mm_mul_ps(xmm,v.xmm));
#endif
    }

    SimdVec4 operator + (const SimdVec4 &v) const
    {
#ifdef SIMD_NEON
        return SimdVec4(vaddq_f32(xmm,v.xmm));
#else
        return SimdVec4(_mm_add_ps(xmm,v.xmm));
#endif
    }

    SimdVec4 operator - (const SimdVec4 &v) const
    {
#ifdef SIMD_NEON
        return SimdVec4(vsubq_f32(xmm,v.xmm));
#else
        return SimdVec4(_mm_sub_ps(xmm,v.xmm));
#endif
    }

    void operator *= (const SimdVec4 &v)
    {
#ifdef SIMD_NEON
        xmm=vmulq_f32(xmm,v.xmm);
#else
        xmm=_mm_mul_ps(xmm,v.xmm);
#endif
    }

    void operator += (const SimdVec4 &v)
    {
#ifdef SIMD_NEON
        xmm=vaddq_f32(xmm, v.xmm);
#else
        xmm=_mm_add_ps(xmm, v.xmm);
#endif
    }

    void operator -= (const SimdVec4 &v)
    {
#ifdef SIMD_NEON
        xmm=vsubq_f32(xmm, v.xmm);
#else
        xmm=_mm_sub_ps(xmm, v.xmm);
#endif
    }
};

}
