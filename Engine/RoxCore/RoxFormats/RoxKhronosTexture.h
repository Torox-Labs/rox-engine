// Updated By the ROX_ENGINE
// Copyright (C) 2024 Torox Project
// Portions Copyright (C) 2013 nyan.developer@gmail.com (nya-engine)
//
// This file was modified by the Torox Project.
// Update the render API interface to check Metal first.
//
// This file incorporates code from the nya-engine project, which is licensed under the MIT License.
// See the LICENSE-MIT file in the root directory for more information.
//
// This file is also part of the Rox-engine, which is licensed under a dual-license system:
// 1. Free Use License (for non-commercial and commercial use under specific conditions)
// 2. Commercial License (for use on proprietary platforms)
// See the LICENSE file in the root directory for the full Rox-engine license terms.

#pragma once

#include <cstddef>

namespace RoxFormats
{

    struct KhronosTexture
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

        const void* data;
        std::size_t data_size;

        KhronosTexture() : width(0), height(0), mipmap_count(0), data(nullptr), data_size(0) {}

    public:
        std::size_t decodeHeader(const void* data, std::size_t size); // Returns 0 if invalid
    };

} // namespace RoxFormats
