//nya-engine (C) nyan.developer@gmail.com released under the MIT license (see LICENSE)

#include "RoxKhronosTexture.h"
#include "RoxMemory/RoxMemoryReader.h"
#include "RoxResources/RoxResources.h"
#include <stdint.h>

namespace RoxFormats
{

    struct KtxHeader
    {
        using uint = uint32_t;
        uint endianess;
        uint gl_type;
        uint gl_type_size;
        uint gl_format;
        uint gl_internal_format;
        uint gl_base_internal_format;
        uint width;
        uint height;
        uint depth;
        uint array_elements_count;
        uint faces_count;
        uint mipmap_count;
        uint key_value_size;
    };

    std::size_t KhronosTexture::decodeHeader(const void* data_ptr, std::size_t size)
    {
        *this = KhronosTexture();

        if (!data_ptr || size < 128)
            return 0;

        using uint = uint32_t;

        RoxMemory::RoxMemoryReader reader(data_ptr, size);
        if (!reader.test("\xABKTX 11\xBB\r\n\x1A\n", 12))
            return 0;

        const KtxHeader header = reader.read<KtxHeader>();
        if (header.endianess != 0x04030201)
            return 0;

        reader.skip(header.key_value_size);

        const bool is_cubemap = header.faces_count == 6;
        if (is_cubemap || header.faces_count != 1)
            return 0;

        if (is_cubemap) // ToDo: Handle cubemap textures
            return 0;

        PIXEL_FORMAT pixel_format;

        switch (header.gl_format)
        {
        case 0x1907: pixel_format = RGB; break;
        case 0x1908: pixel_format = RGBA; break;
        case 0x80E1: pixel_format = BGRA; break;

        case 0:
        {
            switch (header.gl_internal_format)
            {
            case 0x8D64: pixel_format = ETC1; break;
            case 0x9274: pixel_format = ETC2; break;
            case 0x9278: pixel_format = ETC2_EAC; break;
            case 0x9276: pixel_format = ETC2_A1; break;

            case 0x8C01: pixel_format = PVR_RGB2B; break;
            case 0x8C00: pixel_format = PVR_RGB4B; break;
            case 0x8C03: pixel_format = PVR_RGBA2B; break;
            case 0x8C02: pixel_format = PVR_RGBA4B; break;
            default:
                return 0;
            }
        }
        break;

        default:
            return 0;
        }

        if (!header.mipmap_count)
            return 0;

        std::size_t calculated_data_size = 0;
        unsigned int current_width = header.width;
        unsigned int current_height = header.height;

        for (uint i = 0; i < header.mipmap_count; ++i, current_width = (current_width > 1) ? (current_width / 2) : 1, current_height = (current_height > 1) ? (current_height / 2) : 1)
        {
            if (pixel_format < ETC1)
                calculated_data_size += current_width * current_height * ((pixel_format == RGB) ? 3 : 4);
            else if (pixel_format == PVR_RGB2B || pixel_format == PVR_RGBA2B)
                calculated_data_size += ((current_width > 16 ? current_width : 16) * (current_height > 8 ? current_height : 8) * 2 + 7) / 8;
            else if (pixel_format == PVR_RGB4B || pixel_format == PVR_RGBA4B)
                calculated_data_size += ((current_width > 8 ? current_width : 8) * (current_height > 8 ? current_height : 8) * 4 + 7) / 8;
            else
                calculated_data_size += ((current_width + 3) >> 2) * ((current_height + 3) >> 2) * ((pixel_format == ETC2_EAC) ? 16 : 8);
        }

        calculated_data_size += header.mipmap_count * 4;

        if (!reader.checkRemained(calculated_data_size))
            return 0;

        width = header.width;
        height = header.height;
        data_size = calculated_data_size;
        data = reader.getData();
        mipmap_count = header.mipmap_count;
        pf = pixel_format;

        return reader.getOffset();
    }

} // namespace RoxFormats
