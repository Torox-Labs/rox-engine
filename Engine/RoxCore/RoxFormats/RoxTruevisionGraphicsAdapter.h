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

#include <vector>
#include <cstddef>

#include "RoxMath/RoxVector.h"
#include "RoxMath/RoxQuaternion.h"

namespace RoxFormats
{

    struct TGA
    {
        enum COLOR_MODE
        {
            GREYSCALE = 1,
            BGR = 3,
            BGRA = 4
        };

        int width;
        int height;
        COLOR_MODE channels;
        bool rle;
        bool horisontal_flip;
        bool vertical_flip;

        const void* data;
        std::size_t compressed_size;
        std::size_t uncompressed_size;

    public:
        std::size_t decodeHeader(const void* data, std::size_t size); // Returns 0 if invalid
        bool decodeRLE(void* decoded_data) const; // decoded_data must be allocated with uncompressedSize
        void flipHorisontal(const void* from_data, void* to_data); // to_data must be allocated; can be equal to from_data
        void flipVertical(const void* from_data, void* to_data);

    public:
        std::size_t encodeHeader(void* to_data, std::size_t to_size = TGA::header_size);
        std::size_t encodeRLE(void* to_data, std::size_t to_size); // to_data size should be allocated with enough size

    public:
        static const std::size_t header_size = 18;
        
        // TODO: remove legacy
        static const std::size_t minimum_header_size = header_size; 

    public:
        TGA()
            : width(0), height(0), rle(false), horisontal_flip(false),
            vertical_flip(false), data(nullptr), compressed_size(0), uncompressed_size(0) {}
    };

    class TgaFile
    {
    public:
        bool load(const char* file_name);
        bool create(int width, int height, TGA::COLOR_MODE channels, const void* data);
        bool decodeRLE();
        bool encodeRLE(std::size_t max_compressed_size);
        bool save(const char* file_name);
        bool flipHorizontal();
        bool flipVertical();
        void release() { m_header = TGA(); m_data.clear(); }

    public:
        bool isRLE() const { return m_header.rle; }
        bool isFlippedVertical() const { return m_header.vertical_flip; }
        bool isFlippedHorizontal() const { return m_header.horisontal_flip; }
        int getWidth() const { return m_header.width; }
        int getHeight() const { return m_header.height; }
        TGA::COLOR_MODE getChannels() const { return m_header.channels; }
        const unsigned char* getData() const { return m_data.empty() ? nullptr : &m_data[0]; }
        unsigned char* getData() { return m_data.empty() ? nullptr : &m_data[0]; }
        std::size_t getDataSize() const { return m_data.size(); }

    private:
        TGA m_header;
        std::vector<unsigned char> m_data;
    };

}
