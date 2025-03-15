// nya-engine (C) nyan.developer@gmail.com released under the MIT license (see LICENSE)

#include "RoxDirectDrawSurface.h"
#include "RoxMemory/RoxMemoryReader.h"
#include "RoxMemory/RoxTmpBuffers.h"
#include "RoxResources/RoxResources.h"

#include <cstdint>
#include <cstring>
#include <algorithm>

namespace RoxFormats
{

    struct DirectDrawSurfacePixelFormat
    {
        using uint = uint32_t;

        uint size;
        uint flags;
        uint four_cc;
        uint bpp;
        uint bit_mask[4]; // rgba
    };

    static void flipRaw(int width, int height, int channels, const void* from_data, void* to_data)
    {
        if (!height)
            return;

        const std::size_t line_size = width * channels;
        const std::size_t total_size = line_size * height;
        const unsigned char* from = static_cast<const unsigned char*>(from_data);
        unsigned char* to = static_cast<unsigned char*>(to_data) + line_size * (height - 1);

        for (std::size_t offset = 0; offset < total_size; offset += line_size)
            std::memcpy(to - offset, from + offset, line_size);
    }

    static void flipDxt1BlockFull(unsigned char* data)
    {
        std::swap(data[4], data[7]);
        std::swap(data[5], data[6]);
    }

    static void flipDxt3BlockFull(unsigned char* data)
    {
        std::swap(data[0], data[6]);
        std::swap(data[1], data[7]);
        std::swap(data[2], data[4]);
        std::swap(data[3], data[5]);

        flipDxt1BlockFull(data + 8);
    }

    static void flipDxt5BlockFull(unsigned char* data)
    {
        unsigned int line_0_1 = data[2] + 256 * (data[3] + 256 * data[4]);
        unsigned int line_2_3 = data[5] + 256 * (data[6] + 256 * data[7]);

        // Swap lines 0 and 1 in line_0_1.
        unsigned int line_1_0 = ((line_0_1 & 0x000FFF) << 12) |
            ((line_0_1 & 0xFFF000) >> 12);

        // Swap lines 2 and 3 in line_2_3.
        unsigned int line_3_2 = ((line_2_3 & 0x000FFF) << 12) |
            ((line_2_3 & 0xFFF000) >> 12);

        data[2] = line_3_2 & 0xFF;
        data[3] = (line_3_2 & 0xFF00) >> 8;
        data[4] = (line_3_2 & 0xFF0000) >> 16;
        data[5] = line_1_0 & 0xFF;
        data[6] = (line_1_0 & 0xFF00) >> 8;
        data[7] = (line_1_0 & 0xFF0000) >> 16;

        flipDxt1BlockFull(data + 8);
    }

    static void flipDxt(int width, int height, DirectDrawSurface::PIXEL_FORMAT format, const void* from_data, void* to_data)
    {
        if (from_data == to_data) // ToDo: Handle in-place flipping if necessary
            return;

        if (!height)
            return;

        const unsigned int line_size = ((width + 3) / 4) * (format == DirectDrawSurface::DXT1 ? 8 : 16);
        const unsigned char* src = static_cast<const unsigned char*>(from_data);
        unsigned char* dest = static_cast<unsigned char*>(to_data) + ((height + 3) / 4 - 1) * line_size;

        if (height == 1)
        {
            std::memcpy(dest, src, line_size);
            return;
        }

        for (int i = 0; i < (height + 3) / 4; ++i)
        {
            std::memcpy(dest, src, line_size);

            switch (format)
            {
            case DirectDrawSurface::DXT1:
                if (height == 2)
                {
                    for (unsigned int k = 0; k < line_size; k += 8)
                        std::swap(dest[k + 4], dest[k + 5]);
                }
                else
                {
                    for (unsigned int k = 0; k < line_size; k += 8)
                        flipDxt1BlockFull(dest + k);
                }
                break;

            case DirectDrawSurface::DXT2:
            case DirectDrawSurface::DXT3:
                for (unsigned int k = 0; k < line_size; k += 16)
                    flipDxt3BlockFull(dest + k);
                break;

            case DirectDrawSurface::DXT4:
            case DirectDrawSurface::DXT5:
                for (unsigned int k = 0; k < line_size; k += 16)
                    flipDxt5BlockFull(dest + k);
                break;

            default:
                return;
            }

            src += line_size;
            dest -= line_size;
        }
    }

    void DirectDrawSurface::flipVertical(const void* from_data, void* to_data) const
    {
        if (!from_data || !to_data || !height)
            return;

        if (type == TEXTURE_CUBE) // ToDo: Handle cube textures if necessary
            return;

        std::size_t offset = 0;
        unsigned int current_width = width;
        unsigned int current_height = height;

        for (unsigned int i = 0; i < mipmap_count; ++i)
        {
            const void* src_mip = static_cast<const char*>(from_data) + offset;
            void* dest_mip = static_cast<char*>(to_data) + offset;

            switch (pf)
            {
            case RGB:
            case BGR:
            case RGBA:
            case BGRA:
            case GREYSCALE:
            {
                int channels = (pf == BGRA || pf == RGBA) ? 4 :
                    (pf == BGR || pf == RGB) ? 3 : 1;
                flipRaw(current_width, current_height, channels, src_mip, dest_mip);
                offset += current_width * current_height * channels;
                break;
            }

            case DXT1:
            {
                unsigned int size = ((current_width > 4 ? current_width : 4) / 4) *
                    ((current_height > 4 ? current_height : 4) / 4) * 8;
                flipDxt(current_width, current_height, pf, src_mip, dest_mip);
                offset += size;
                break;
            }

            case DXT2:
            case DirectDrawSurface::DXT3:
            case DirectDrawSurface::DXT4:
            case DirectDrawSurface::DXT5:
            {
                unsigned int size = ((current_width > 4 ? current_width : 4) / 4) *
                    ((current_height > 4 ? current_height : 4) / 4) * 16;
                flipDxt(current_width, current_height, pf, src_mip, dest_mip);
                offset += size;
                break;
            }

            case PALETTE4_RGBA:
            case PALETTE8_RGBA:
            {
                // ToDo: Not tested
                return;

                int palette_offset = (pf == PALETTE8_RGBA) ? 256 * 4 : 16 * 4;
                flipRaw(current_width, current_height, 1,
                    static_cast<const char*>(src_mip) + palette_offset,
                    static_cast<char*>(dest_mip) + palette_offset);
                break;
            }

            default:
                return;
            }

            current_width = (current_width > 1) ? (current_width / 2) : 1;
            current_height = (current_height > 1) ? (current_height / 2) : 1;
        }
    }

    std::size_t DirectDrawSurface::getDecodedSize() const
    {
        std::size_t size = 0;
        unsigned int current_width = width;
        unsigned int current_height = height;

        for (unsigned int i = 0; i < mipmap_count; ++i)
        {
            size += current_width * current_height * 4;
            current_width = (current_width > 1) ? (current_width / 2) : 1;
            current_height = (current_height > 1) ? (current_height / 2) : 1;
        }

        return (type == TEXTURE_CUBE) ? size * 6 : size;
    }

    void DirectDrawSurface::decodePalette8Rgba(void* decoded_data) const
    {
        if (pf != PALETTE8_RGBA)
            return;

        // Use memcpy instead of cast to avoid misalignment
        unsigned int palette[256];
        std::memcpy(palette, data, sizeof(palette));

        const unsigned char* indices = static_cast<const unsigned char*>(data) + sizeof(palette);
        unsigned char* out = static_cast<unsigned char*>(decoded_data); // Alignment assumed

        for (unsigned int i = 0; i < width * height; ++i, ++indices, out += 4)
            std::memcpy(out, &palette[*indices], 4);
    }

    inline int unpack565(const unsigned char* src, unsigned char* dst)
    {
        using uchar = unsigned char;

        int value = static_cast<int>(src[0]) | (static_cast<int>(src[1]) << 8);

        uchar r = static_cast<uchar>((value >> 11) & 0x1F);
        uchar g = static_cast<uchar>((value >> 5) & 0x3F);
        uchar b = static_cast<uchar>(value & 0x1F);

        dst[0] = (r << 3) | (r >> 2);
        dst[1] = (g << 2) | (g >> 4);
        dst[2] = (b << 3) | (b >> 2);
        dst[3] = 255;

        return value;
    }

    inline void decompressColor(const void* src, void* dst, bool is_dxt1)
    {
        using uchar = unsigned char;

        const uchar* src_buf = static_cast<const uchar*>(src);
        if (!is_dxt1)
            src_buf += 8;

        uchar codes[16];
        int a = unpack565(src_buf, codes);
        int b = unpack565(src_buf + 2, codes + 4);

        for (int i = 0; i < 3; ++i)
        {
            int c = codes[i];
            int d = codes[i + 4];

            if (is_dxt1 && a <= b)
            {
                codes[i + 8] = static_cast<uchar>((c + d) / 2);
                codes[i + 12] = 0;
            }
            else
            {
                codes[i + 8] = static_cast<uchar>((c * 2 + d) / 3);
                codes[i + 12] = static_cast<uchar>((c + d * 2) / 3);
            }
        }

        codes[11] = 255;
        codes[15] = (is_dxt1 && a <= b) ? 0 : 255;

        uchar indices[16];
        for (int i = 0; i < 4; ++i)
        {
            uchar packed = src_buf[i + 4];
            indices[i * 4 + 0] = packed & 0x3;
            indices[i * 4 + 1] = (packed >> 2) & 0x3;
            indices[i * 4 + 2] = (packed >> 4) & 0x3;
            indices[i * 4 + 3] = (packed >> 6) & 0x3;
        }

        for (int i = 0; i < 16; ++i)
        {
            uchar offset = indices[i] * 4;
            for (int j = 0; j < 4; ++j)
                static_cast<uchar*>(dst)[i * 4 + j] = codes[offset + j];
        }
    }

    inline void decompressDxt3Alpha(const void* src, void* dst)
    {
        using uchar = unsigned char;

        const uchar* src_buf = static_cast<const uchar*>(src);
        uchar* dst_buf = static_cast<uchar*>(dst);

        for (int i = 0; i < 64; i += 8, ++src_buf)
        {
            uchar lo = *src_buf & 0x0F;
            uchar hi = *src_buf & 0xF0;
            dst_buf[i + 3] = lo | (lo << 4);
            dst_buf[i + 7] = hi | (hi >> 4);
        }
    }

    inline void decompressDxt5Alpha(const void* src, void* dst)
    {
        using uchar = unsigned char;

        const uchar* src_buf = static_cast<const uchar*>(src);
        uchar* dst_buf = static_cast<uchar*>(dst);

        int alpha0 = src_buf[0];
        int alpha1 = src_buf[1];

        uchar codes[8];
        codes[0] = src_buf[0];
        codes[1] = src_buf[1];
        if (alpha0 <= alpha1)
        {
            for (int i = 1; i < 5; ++i)
                codes[i + 1] = static_cast<uchar>(((5 - i) * alpha0 + i * alpha1) / 5);

            codes[6] = 0;
            codes[7] = 255;
        }
        else
        {
            for (int i = 1; i < 7; ++i)
                codes[i + 1] = static_cast<uchar>(((7 - i) * alpha0 + i * alpha1) / 7);
        }

        src_buf += 2;
        uchar indices[16];
        uchar* dest = indices;
        for (int i = 0; i < 2; ++i)
        {
            unsigned int value = 0;
            for (int j = 0; j < 3; ++j)
                value |= (static_cast<unsigned int>(*src_buf++) << (8 * j));

            for (int j = 0; j < 8; ++j)
                *dest++ = static_cast<uchar>((value >> (3 * j)) & 0x7);
        }

        for (int i = 0; i < 16; ++i)
            dst_buf[4 * i + 3] = codes[indices[i]];
    }

    void DirectDrawSurface::decodeDxt(void* decoded_data) const
    {
        using uint = unsigned int;
        const char* src_buf = static_cast<const char*>(data);
        const uint bytes_per_block = (pf == DXT1) ? 8 : 16;

        for (int f = 0; f < ((type == TEXTURE_CUBE) ? 6 : 1); ++f)
        {
            unsigned int current_width = width;
            unsigned int current_height = height;

            for (uint i = 0; i < mipmap_count; ++i)
            {
                for (uint y = 0; y < current_height; y += 4)
                {
                    for (uint x = 0; x < current_width; x += 4)
                    {
                        uint rgba[16];

                        switch (pf)
                        {
                        case DXT1:
                            decompressColor(src_buf, rgba, true);
                            break;

                        case DXT2:
                        case DXT3:
                            decompressColor(src_buf, rgba, false);
                            decompressDxt3Alpha(src_buf, rgba);
                            break;

                        case DXT4:
                        case DXT5:
                            decompressColor(src_buf, rgba, false);
                            decompressDxt5Alpha(src_buf, rgba);
                            break;

                        default:
                            return;
                        }

                        for (uint py = 0, sy = y; py < 16 && sy < current_height; py += 4, ++sy)
                            std::memcpy(static_cast<uint*>(decoded_data) + current_width * sy + x,
                                &rgba[py],
                                ((x + 4 < current_width) ? 4 : (current_width - x)) * sizeof(uint));

                        src_buf += bytes_per_block;
                    }
                }

                decoded_data = static_cast<char*>(decoded_data) + (current_width * current_height) * 4;
                current_width = (current_width > 1) ? (current_width / 2) : 1;
                current_height = (current_height > 1) ? (current_height / 2) : 1;
            }
        }
    }

    std::size_t DirectDrawSurface::decodeHeader(const void* data_ptr, std::size_t size)
    {
        *this = DirectDrawSurface();

        if (!data_ptr || size < 128)
            return 0;

        using uint = uint32_t;

        RoxMemory::RoxMemoryReader reader(data_ptr, size);
        if (!reader.test("DDS ", 4))
            return 0;

        if (reader.read<uint>() != 124)
            return 0;

        const uint DDS_PIXELFORMAT = 0x00001000;
        const uint DDS_CAPS = 0x00000001;

        uint flags = reader.read<uint>();
        if (!(flags & DDS_PIXELFORMAT) || !(flags & DDS_CAPS))
            return 0;

        height = reader.read<uint>();
        width = reader.read<uint>();
        reader.skip(4); // Skipping pitch
        uint depth = reader.read<uint>();
        if (depth > 1) // ToDo: Handle 3D textures
            return 0;

        mipmap_count = reader.read<uint>();
        if (!mipmap_count)
        {
            need_generate_mipmaps = true;
            mipmap_count = 1;
        }

        reader.skip(44);

        const uint DDS_FOURCC = 0x00000004;
        const uint DDS_PALETTE4 = 0x00000008;
        const uint DDS_PALETTE8 = 0x00000020;
        const uint DDS_CUBEMAP = 0x00000200;

        DirectDrawSurfacePixelFormat pf_format = reader.read<DirectDrawSurfacePixelFormat>();
        if (pf_format.flags & DDS_FOURCC)
        {
            switch (pf_format.four_cc)
            {
            case 0x31545844: // '1TXD'
                this->pf = DXT1;
                break;
            case 0x32545844: // '2TXD'
                this->pf = DXT2;
                break;
            case 0x33545844: // '3TXD'
                this->pf = DXT3;
                break;
            case 0x34545844: // '4TXD'
                this->pf = DXT4;
                break;
            case 0x35545844: // '5TXD'
                this->pf = DXT5;
                break;
            default:
                return 0;
            }

            for (uint i = 0, w = width, h = height; i < mipmap_count; ++i, w /= 2, h /= 2)
            {
                uint size = ((w > 4 ? w : 4) / 4) * ((h > 4 ? h : 4) / 4) * ((this->pf == DXT1) ? 8 : 16);
                this->data_size += size;
            }
        }
        else
        {
            if (pf_format.bpp == 32)
            {
                this->pf = (pf_format.bit_mask[0] == 255) ? RGBA : BGRA;
            }
            else if (pf_format.bpp == 24)
            {
                this->pf = (pf_format.bit_mask[0] == 255) ? RGB : BGR;
            }
            else if (pf_format.bpp == 8)
            {
                if (pf_format.flags & DDS_PALETTE8)
                    this->pf = PALETTE8_RGBA;
                else if (pf_format.flags & DDS_PALETTE4)
                    this->pf = PALETTE4_RGBA;
                else
                    this->pf = GREYSCALE;
            }
            else
            {
                return 0;
            }

            for (uint i = 0, w = width, h = height; i < mipmap_count; ++i, w = (w > 1) ? (w / 2) : 1, h = (h > 1) ? (h / 2) : 1)
                this->data_size += w * h * (pf_format.bpp / 8);
        }

        reader.read<uint>(); // Skipping caps
        uint caps2 = reader.read<uint>();

        type = TEXTURE_2D;
        if (caps2 & DDS_CUBEMAP)
        {
            type = TEXTURE_CUBE;
            this->data_size *= 6;
        }

        reader.seek(128);
        if (!reader.checkRemained(this->data_size))
        {
            this->mipmap_count = static_cast<unsigned int>(-1);
            // Probably broken, try to load at least the first mipmap
            if (!reader.checkRemained(this->data_size))
                return 0;
        }

        this->data = reader.getData();

        return reader.getOffset();
    }

    std::size_t DirectDrawSurface::getMipSize(int mip_idx) const
    {
        if (mip_idx < 0 || mip_idx >= static_cast<int>(mipmap_count))
            return 0;

        unsigned int w1 = width >> mip_idx;
        unsigned int h1 = height >> mip_idx;
        w1 = (w1 > 1) ? w1 : 1;
        h1 = (h1 > 1) ? h1 : 1;

        if (pf <= DXT5)
        {
            return ((w1 > 4 ? w1 : 4) / 4) * ((h1 > 4 ? h1 : 4) / 4) *
                ((pf == DXT1) ? 8 : 16) *
                ((type == TEXTURE_CUBE) ? 6 : 1);
        }

        return (w1) * (h1) *
            ((pf == BGRA) ? 4 : ((pf == BGR) ? 3 : 1)) *
            ((type == TEXTURE_CUBE) ? 6 : 1);
    }

}
