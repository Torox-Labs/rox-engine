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

#include "RoxAngle.h"
#include "RoxScalar.h"

namespace RoxMath
{
	struct Vector2
	{
		float x, y;

		Vector2(): x(0), y(0)
		{
		}

		Vector2(float x, float y) { this->x = x, this->y = y; }
		explicit Vector2(const float* v) { x = v[0], y = v[1]; }

		Vector2& set(float x, float y)
		{
			this->x = x, this->y = y;
			return *this;
		}

		Vector2 operator +(const Vector2& v) const { return Vector2(x + v.x, y + v.y); }
		Vector2 operator -(const Vector2& v) const { return Vector2(x - v.x, y - v.y); }
		Vector2 operator *(const Vector2& v) const { return Vector2(x * v.x, y * v.y); }
		Vector2 operator *(const float a) const { return Vector2(x * a, y * a); }
		Vector2 operator /(const Vector2& v) const { return Vector2(x / v.x, y / v.y); }
		Vector2 operator /(const float a) const { return Vector2(x / a, y / a); }

		Vector2 operator -() const { return Vector2(-x, -y); }

		Vector2 operator *=(const Vector2& v)
		{
			x *= v.x;
			y *= v.y;
			return *this;
		}

		Vector2 operator *=(const float a)
		{
			x *= a;
			y *= a;
			return *this;
		}

		Vector2 operator /=(const float a)
		{
			x /= a;
			y /= a;
			return *this;
		}

		Vector2 operator +=(const Vector2& v)
		{
			x += v.x;
			y += v.y;
			return *this;
		}

		Vector2 operator -=(const Vector2& v)
		{
			x -= v.x;
			y -= v.y;
			return *this;
		}

		float length() const { return sqrtf(lengthSq()); }
		float lengthSq() const { return dot(*this); }

		float dot(const Vector2& v) const { return dot(*this, v); }
		Vector2& normalize() { return *this = normalize(*this); }

		Vector2& abs()
		{
			x = fabsf(x);
			y = fabsf(y);
			return *this;
		}

		Vector2& clamp(const Vector2& from, const Vector2& to) { return *this = clamp(*this, from, to); }
		Vector2 reflect(const Vector2& n) const { return reflect(*this, n); }
		AngleRad angle(const Vector2& v) const { return angle(*this, v); }
		Vector2& rotate(AngleRad a) { return *this = rotate(*this, a); }

		AngleRad getAngle() const { return (x == 0.0f && y == 0.0f) ? 0.0f : atan2(x, y); }

		static Vector2 max(const Vector2& a, const Vector2& b) { return Vector2(RoxMath::max(a.x, b.x), RoxMath::max(a.y, b.y)); }
		static Vector2 min(const Vector2& a, const Vector2& b) { return Vector2(RoxMath::min(a.x, b.x), RoxMath::min(a.y, b.y)); }

		static float dot(const Vector2& a, const Vector2& b) { return a.x * b.x + a.y * b.y; }

		static Vector2 normalize(const Vector2& v)
		{
			float len = v.length();
			return len < 1.0e-6f ? Vector2(1.0f, 0.0f) : v * (1.0f / len);
		}

		static Vector2 abs(const Vector2& v) { return Vector2(fabsf(v.x), fabsf(v.y)); }
		static Vector2 lerp(const Vector2& from, const Vector2& to, float t) { return from + (to - from) * t; }
		static Vector2 clamp(const Vector2& v, const Vector2& from, const Vector2& to) { return min(max(v, from), to); }

		static Vector2 reflect(const Vector2& v, const Vector2& n)
		{
			const float d = dot(v, n);
			return v - n * (d + d);
		}

		static AngleRad angle(const Vector2& a, const Vector2& b) { return acos(dot(normalize(a), normalize(b))); }

		static Vector2 rotate(const Vector2& v, AngleRad a)
		{
			const float c = cos(a), s = sin(a);
			return Vector2(c * v.x - s * v.y, s * v.x + c * v.y);
		}

		static const Vector2& right()
		{
			static Vector2 v(1.0f, 0.0f);
			return v;
		}

		static const Vector2& up()
		{
			static Vector2 v(0.0f, 1.0f);
			return v;
		}

		static const Vector2& zero()
		{
			static Vector2 v(0.0f, 0.0f);
			return v;
		}

		static const Vector2& one()
		{
			static Vector2 v(1.0f, 1.0f);
			return v;
		}
	};

	inline Vector2 operator *(float a, const Vector2& v) { return Vector2(a * v.x, a * v.y); }
	inline Vector2 operator /(float a, const Vector2& v) { return Vector2(a / v.x, a / v.y); }

	struct Vector3
	{
		float x, y, z;

		Vector3(): x(0), y(0), z(0)
		{
		}

		Vector3(float x, float y, float z) { this->x = x, this->y = y, this->z = z; }
		Vector3(const RoxMath::Vector2& v, float z) { this->x = v.x, this->y = v.y, this->z = z; }
		explicit Vector3(const float* v) { x = v[0], y = v[1], z = v[2]; }

		Vector3& set(float x, float y, float z)
		{
			this->x = x, this->y = y, this->z = z;
			return *this;
		}

		Vector3& set(const RoxMath::Vector2& v, float z)
		{
			this->x = v.x, this->y = v.y, this->z = z;
			return *this;
		}

		Vector3 operator +(const Vector3& v) const { return Vector3(x + v.x, y + v.y, z + v.z); }
		Vector3 operator -(const Vector3& v) const { return Vector3(x - v.x, y - v.y, z - v.z); }
		Vector3 operator *(const Vector3& v) const { return Vector3(x * v.x, y * v.y, z * v.z); }
		Vector3 operator *(const float a) const { return Vector3(x * a, y * a, z * a); }
		Vector3 operator /(const Vector3& v) const { return Vector3(x / v.x, y / v.y, z / v.z); }
		Vector3 operator /(const float a) const { return Vector3(x / a, y / a, z / a); }

		Vector3 operator -() const { return Vector3(-x, -y, -z); }

		Vector3 operator *=(const Vector3& v)
		{
			x *= v.x;
			y *= v.y;
			z *= v.z;
			return *this;
		}

		Vector3 operator *=(const float a)
		{
			x *= a;
			y *= a;
			z *= a;
			return *this;
		}

		Vector3 operator /=(const float a)
		{
			x /= a;
			y /= a;
			z /= a;
			return *this;
		}

		Vector3 operator +=(const Vector3& v)
		{
			x += v.x;
			y += v.y;
			z += v.z;
			return *this;
		}

		Vector3 operator -=(const Vector3& v)
		{
			x -= v.x;
			y -= v.y;
			z -= v.z;
			return *this;
		}

		float length() const { return sqrtf(lengthSq()); }
		float lengthSq() const { return dot(*this); }

		float dot(const Vector3& v) const { return dot(*this, v); }
		Vector3 cross(const Vector3& v) const { return cross(*this, v); }
		Vector3& normalize() { return *this = normalize(*this); }

		Vector3& abs()
		{
			x = fabsf(x);
			y = fabsf(y);
			z = fabsf(z);
			return *this;
		}

		Vector3& clamp(const Vector3& from, const Vector3& to) { return *this = clamp(*this, from, to); }
		Vector3 reflect(const Vector3& n) const { return reflect(*this, n); }
		AngleRad angle(const Vector3& v) const { return angle(*this, v); }

		const Vector2& xy() const { return *(Vector2*)&x; }
		Vector2& xy() { return *(Vector2*)&x; }

		AngleRad getYaw() const { return (x == 0.0f && z == 0.0f) ? 0.0f : atan2(-x, -z); }

		AngleRad getPitch() const
		{
			const float l = length(), eps = 1.0e-6f;
			if (l > eps)
				return -asin(y / l);
			if (y > eps)
				return -asin(1.0f);
			if (y < -eps)
				return asin(1.0f);
			return AngleRad();
		}

		static Vector3 max(const Vector3& a, const Vector3& b)
		{
			return Vector3(RoxMath::max(a.x, b.x), RoxMath::max(a.y, b.y), RoxMath::max(a.z, b.z));
		}

		static Vector3 min(const Vector3& a, const Vector3& b)
		{
			return Vector3(RoxMath::min(a.x, b.x), RoxMath::min(a.y, b.y), RoxMath::min(a.z, b.z));
		}

		static float dot(const Vector3& a, const Vector3& b) { return a.x * b.x + a.y * b.y + a.z * b.z; }

		static Vector3 cross(const Vector3& a, const Vector3& b)
		{
			return Vector3(a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x);
		}

		static Vector3 normalize(const Vector3& v)
		{
			float len = v.length();
			return len < 1.0e-6f ? Vector3(1.0f, 0.0f, 0.0f) : v * (1.0f / len);
		}

		static Vector3 abs(const Vector3& v) { return Vector3(fabsf(v.x), fabsf(v.y), fabsf(v.z)); }
		static Vector3 lerp(const Vector3& from, const Vector3& to, float t) { return from + (to - from) * t; }
		static Vector3 clamp(const Vector3& v, const Vector3& from, const Vector3& to) { return min(max(v, from), to); }

		static Vector3 reflect(const Vector3& v, const Vector3& n)
		{
			const float d = dot(v, n);
			return v - n * (d + d);
		}

		static AngleRad angle(const Vector3& a, const Vector3& b) { return acos(dot(normalize(a), normalize(b))); }

		static const Vector3& forward()
		{
			static Vector3 v(0.0f, 0.0f, -1.0f);
			return v;
		}

		static const Vector3& right()
		{
			static Vector3 v(1.0f, 0.0f, 0.0f);
			return v;
		}

		static const Vector3& up()
		{
			static Vector3 v(0.0f, 1.0f, 0.0f);
			return v;
		}

		static const Vector3& zero()
		{
			static Vector3 v(0.0f, 0.0f, 0.0f);
			return v;
		}

		static const Vector3& one()
		{
			static Vector3 v(1.0f, 1.0f, 1.0f);
			return v;
		}
	};

	inline Vector3 operator *(float a, const Vector3& v) { return Vector3(a * v.x, a * v.y, a * v.z); }
	inline Vector3 operator /(float a, const Vector3& v) { return Vector3(a / v.x, a / v.y, a / v.z); }

	struct Vector4
	{
		float x, y, z, w;

		Vector4(): x(0), y(0), z(0), w(0)
		{
		}

		Vector4(float x, float y, float z, float w) { this->x = x, this->y = y, this->z = z, this->w = w; }
		Vector4(const Vector3& v, float w) { this->x = v.x, this->y = v.y, this->z = v.z, this->w = w; }
		explicit Vector4(const float* v) { x = v[0], y = v[1], z = v[2], w = v[3]; }

		Vector4& set(float x, float y, float z, float w)
		{
			this->x = x, this->y = y, this->z = z, this->w = w;
			return *this;
		}

		Vector4& set(const RoxMath::Vector3& v, float w)
		{
			this->x = v.x, this->y = v.y, this->z = v.z, this->w = w;
			return *this;
		}

		Vector4 operator +(const Vector4& v) const { return Vector4(x + v.x, y + v.y, z + v.z, w + v.w); }

		Vector4 operator -(const Vector4& v) const { return Vector4(x - v.x, y - v.y, z - v.z, w - v.w); }
		Vector4 operator *(const Vector4& v) const { return Vector4(x * v.x, y * v.y, z * v.z, w * v.w); }
		Vector4 operator *(const float a) const { return Vector4(x * a, y * a, z * a, w * a); }
		Vector4 operator /(const Vector4& v) const { return Vector4(x / v.x, y / v.y, z / v.z, w / v.w); }
		Vector4 operator /(const float a) const { return Vector4(x / a, y / a, z / a, w / a); }

		Vector4 operator -() const { return Vector4(-x, -y, -z, -w); }

		Vector4 operator *=(const Vector4& v)
		{
			x *= v.x;
			y *= v.y;
			z *= v.z;
			w *= v.w;
			return *this;
		}

		Vector4 operator *=(const float a)
		{
			x *= a;
			y *= a;
			z *= a;
			w *= a;
			return *this;
		}

		Vector4 operator /=(const float a)
		{
			x /= a;
			y /= a;
			z /= a;
			w /= a;
			return *this;
		}

		Vector4 operator +=(const Vector4& v)
		{
			x += v.x;
			y += v.y;
			z += v.z;
			w += v.w;
			return *this;
		}

		Vector4 operator -=(const Vector4& v)
		{
			x -= v.x;
			y -= v.y;
			z -= v.z;
			w += v.w;
			return *this;
		}

		float length() const { return sqrtf(lengthSq()); }
		float lengthSq() const { return dot(*this); }

		float dot(const Vector4& v) const { return dot(*this, v); }
		Vector4& normalize() { return *this = normalize(*this); }

		Vector4& abs()
		{
			x = fabsf(x);
			y = fabsf(y);
			z = fabsf(z);
			w = fabsf(w);
			return *this;
		}

		const Vector3& xyz() const { return *(Vector3*)&x; }
		Vector3& xyz() { return *(Vector3*)&x; }
		const Vector2& xy() const { return *(Vector2*)&x; }
		Vector2& xy() { return *(Vector2*)&x; }
		const Vector2& zw() const { return *(Vector2*)&z; }
		Vector2& zw() { return *(Vector2*)&z; }

		static Vector4 max(const Vector4& a, const Vector4& b)
		{
			return Vector4(RoxMath::max(a.x, b.x), RoxMath::max(a.y, b.y),
			            RoxMath::max(a.z, b.z), RoxMath::max(a.w, b.w));
		}

		static Vector4 min(const Vector4& a, const Vector4& b)
		{
			return Vector4(RoxMath::min(a.x, b.x), RoxMath::min(a.y, b.y),
			            RoxMath::min(a.z, b.z), RoxMath::min(a.w, b.w));
		}

		static float dot(const Vector4& a, const Vector4& b) { return a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w; }

		static Vector4 normalize(const Vector4& v)
		{
			float len = v.length();
			return len < 1.0e-6f ? Vector4(1.0f, 0.0f, 0.0f, 0.0f) : v * (1.0f / len);
		}

		static Vector4 abs(const Vector4& v) { return Vector4(fabsf(v.x), fabsf(v.y), fabsf(v.z), fabsf(v.w)); }
		static Vector4 lerp(const Vector4& from, const Vector4& to, float t) { return from + (to - from) * t; }
		static Vector4 clamp(const Vector4& v, const Vector4& from, const Vector4& to) { return min(max(v, from), to); }

		static const Vector4& zero()
		{
			static Vector4 v(0.0f, 0.0f, 0.0f, 0.0f);
			return v;
		}

		static const Vector4& one()
		{
			static Vector4 v(1.0f, 1.0f, 1.0f, 1.0f);
			return v;
		}
	};

	inline Vector4 operator *(float a, const Vector4& v) { return Vector4(a * v.x, a * v.y, a * v.z, a * v.w); }
	inline Vector4 operator /(float a, const Vector4& v) { return Vector4(a / v.x, a / v.y, a / v.z, a / v.w); }

}

