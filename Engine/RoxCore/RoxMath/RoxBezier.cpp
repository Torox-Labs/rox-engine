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

#include "RoxBezier.h"
#include <cmath>

namespace RoxMath
{
	RoxBezier::RoxBezier(float x1, float y1, float x2, float y2)
	{
		const float eps = 0.001f;
		if (fabs(x1 - y1) < eps && fabs(x2 - y2) < eps)
		{
			m_linear = true;
			return;
		}

		x1 *= 3.0f;
		y1 *= 3.0f;

		x2 *= 3.0f;
		y2 *= 3.0f;

		m_y[0] = 0.0f;
		m_y[div_count] = 1.0f;

		float dx = 1.0f / (float)div_count;

		for (int i = 1; i < div_count; ++i)
			m_y[i] = getY(dx * i, x1, y1, x2, y2);

		m_linear = false;
	}

	float RoxBezier::get(float x) const
	{
		if (m_linear)
			return x;

		x *= (float)div_count;

		const int idx = (int)x;

		if (idx >= div_count)
			return m_y[div_count];

		if (idx < 0)
			return m_y[0];

		x -= idx;

		return m_y[idx] * (1.0f - x) + m_y[idx + 1] * x;
	}

	float RoxBezier::getY(float x, float x1, float y1, float x2, float y2)
	{
		float t = x;
		float inv_t = 1.0f - t;

		const float eps = 0.001f;

		for (int i = 0; i < 32; ++i)
		{
			float tx = inv_t * inv_t * t * x1 + inv_t * t * t * x2 + t * t * t - x;

			if (fabsf(tx) < eps)
				break;

			t -= 0.5f * tx;
			inv_t = 1.0f - t;
		}

		return inv_t * inv_t * t * y1 + inv_t * t * t * y2 + t * t * t;
	}
}
