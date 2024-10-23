// nya-engine (C) nyan.developer@gmail.com released under the MIT license (see LICENSE)

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
