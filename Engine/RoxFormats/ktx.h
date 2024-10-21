// Updated By the ROX_ENGINE
// Copyright (C) 2024 Torox Project
// Portions Copyright (C) 2013 nyan.developer@gmail.com (nya-engine)
//
// This file was modified by the Torox Project.
// Update the render api intefrace to check Metal 1th.
//
// This file incorporates code from the nya-engine project, which is licensed under the MIT License.
// See the LICENSE-MIT file in the root directory for more information.
//
// This file is also part of the Rox-engine, which is licensed under a dual-license system:
// 1. Free Use License (for non-commercial and commercial use under specific conditions)
// 2. Commercial License (for use on proprietary platforms)
// See the LICENSE file in the root directory for the full Rox-engine license terms.

#pragma once

#include <stddef.h>

namespace nya_formats
{

struct ktx
{
    unsigned int width;
    unsigned int height;

    unsigned int mipmap_count;

    enum PIXEL_FORMAT
    {
	    RGB,
		RGBA,
		BGRA,

		ETC1,
		ETC2,
		ETC2_EAC,
		ETC2_A1,

		PVR_RGB2B,
		PVR_RGB4B,
		PVR_RGBA2B,
		PVR_RGBA4B,
    };

	PIXEL_FORMAT pf;

    const void *data;
    size_t data_size;

    ktx(): width(0),height(0),mipmap_count(0),data(0),data_size(0) {}

public:
    size_t decode_header(const void *data,size_t size); //0 if invalid
};

}
