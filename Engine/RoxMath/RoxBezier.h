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

namespace RoxMath
{
	class RoxBezier
	{
	public:
		float get(float x) const;

	public:
		RoxBezier(): m_linear(true)
		{
		}

		RoxBezier(float x1, float y1, float x2, float y2);

	private:
		float getY(float x, float x1, float y1, float x2, float y2);

	private:
		static const int div_count = 16;
		float m_y[div_count + 1];

		bool m_linear;
	};
}
