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

#include "RoxRender/RoxRender.h"
#include "RoxMath/RoxVector.h"
#include <string>

namespace RoxFormats
{

	bool boolFromString(const char* s);
	RoxMath::Vector4 vec4FromString(const char* s);
	bool cullFaceFromString(const char* s, RoxRender::CullFace::ORDER& orderOut);
	RoxRender::Blend::MODE blendModeFromString(const char* s);
	bool blendModeFromString(const char* s, RoxRender::Blend::MODE& srcOut, RoxRender::Blend::MODE& dstOut);

} // namespace RoxFormats
