//nya-engine (C) nyan.developer@gmail.com released under the MIT license (see LICENSE)

#pragma once

#include "render/render.h"
#include "math/vector.h"
#include <string>

namespace nya_formats
{

bool bool_from_string(const char *s);
nya_math::vec4 vec4_from_string(const char *s);
bool cull_face_from_string(const char *s,nya_render::cull_face::order &order_out);
nya_render::blend::mode blend_mode_from_string(const char *s);
bool blend_mode_from_string(const char *s,nya_render::blend::mode &src_out,nya_render::blend::mode &dst_out);

}
