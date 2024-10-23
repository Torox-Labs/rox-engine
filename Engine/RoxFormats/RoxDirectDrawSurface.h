// nya-engine (C) nyan.developer@gmail.com released under the MIT license (see LICENSE)

#pragma once

#include <cstddef>

namespace RoxFormats
{

    struct DirectDrawSurface
    {
        unsigned int width;
        unsigned int height;

        unsigned int mipmap_count;
        bool need_generate_mipmaps;

        enum TEXTURE_TYPE
        {
            TEXTURE_2D,
            TEXTURE_CUBE
        };

        TEXTURE_TYPE type;

        enum PIXEL_FORMAT
        {
            DXT1,
            DXT2,
            DXT3,
            DXT4,
            DXT5,
            RGBA,
            BGRA,
            RGB,
            BGR,
            PALETTE4_RGBA,
            PALETTE8_RGBA,
            GREYSCALE
        };

        PIXEL_FORMAT pf;

        const void* data;
        std::size_t data_size;

        DirectDrawSurface() : width(0), height(0), mipmap_count(0), need_generate_mipmaps(false),
            data(nullptr), data_size(0) {}

    public:
        std::size_t decodeHeader(const void* data, std::size_t size); // Returns 0 if invalid
        void flipVertical(const void* from_data, void* to_data) const;
        std::size_t getMipSize(int mip_idx) const;

        std::size_t getDecodedSize() const;
        void decodePalette8Rgba(void* decoded_data) const; // Requires width*height*4 buffer
        void decodeDxt(void* decoded_data) const; // Requires allocation with getDecodedSize()
    };

} // namespace RoxFormats
