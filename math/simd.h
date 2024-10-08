//nya-engine (C) nyan.developer@gmail.com released under the MIT license (see LICENSE)

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

namespace nya_math
{

struct align16 simd_vec4
{
#ifdef SIMD_NEON
    float32x4_t xmm;
#else
    __m128 xmm;
#endif

    simd_vec4()
    {
#ifdef SIMD_NEON
        xmm=vdupq_n_f32(0.0f);
#else
        xmm=_mm_set1_ps(0.0f);
#endif
    }

#ifdef SIMD_NEON
    simd_vec4(const float32x4_t xmm): xmm(xmm) {}
#else
    simd_vec4(const __m128 xmm): xmm(xmm) {}
#endif

    simd_vec4(const float *v)
    {
#ifdef SIMD_NEON
        xmm=vld1q_f32(v);
#else
        xmm=_mm_loadu_ps(v);
#endif
    }

    simd_vec4(float v)
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

    simd_vec4 operator * (const simd_vec4 &v) const
    {
#ifdef SIMD_NEON
        return simd_vec4(vmulq_f32(xmm,v.xmm));
#else
        return simd_vec4(_mm_mul_ps(xmm,v.xmm));
#endif
    }

    simd_vec4 operator + (const simd_vec4 &v) const
    {
#ifdef SIMD_NEON
        return simd_vec4(vaddq_f32(xmm,v.xmm));
#else
        return simd_vec4(_mm_add_ps(xmm,v.xmm));
#endif
    }

    simd_vec4 operator - (const simd_vec4 &v) const
    {
#ifdef SIMD_NEON
        return simd_vec4(vsubq_f32(xmm,v.xmm));
#else
        return simd_vec4(_mm_sub_ps(xmm,v.xmm));
#endif
    }

    void operator *= (const simd_vec4 &v)
    {
#ifdef SIMD_NEON
        xmm=vmulq_f32(xmm,v.xmm);
#else
        xmm=_mm_mul_ps(xmm,v.xmm);
#endif
    }

    void operator += (const simd_vec4 &v)
    {
#ifdef SIMD_NEON
        xmm=vaddq_f32(xmm, v.xmm);
#else
        xmm=_mm_add_ps(xmm, v.xmm);
#endif
    }

    void operator -= (const simd_vec4 &v)
    {
#ifdef SIMD_NEON
        xmm=vsubq_f32(xmm, v.xmm);
#else
        xmm=_mm_sub_ps(xmm, v.xmm);
#endif
    }
};

}
