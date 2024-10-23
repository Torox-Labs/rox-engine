// nya-engine (C) nyan.developer@gmail.com released under the MIT license (see LICENSE)

#include "RoxTexture.h"
#include "RoxScene.h"
#include "RoxMemory/RoxMemoryReader.h"
#include "RoxMemory/RoxTmpBuffers.h"
#include "RoxFormats/RoxTruevisionGraphicsAdapter.h"
#include "RoxFormats/RoxDirectDrawSurface.h"
#include "RoxFormats/RoxKhronosTexture.h"
#include "RoxFormats/RoxMeta.h"
#include "RoxRender/RoxFbo.h"
#include "RoxRender/RoxShader.h"
#include "RoxRender/RoxScreenQuad.h"
#include "RoxRender/RoxBitmap.h"
#include <stdlib.h>

namespace RoxScene
{

    int RoxTexture::m_load_ktx_mip_offset = 0;

    bool RoxTexture::loadKtx(SharedTexture& res, RoxResources::RoxResourceData& data, const char* name)
    {
        if (!data.getSize())
            return false;

        if (data.getSize() < 12)
            return false;

        if (memcmp(reinterpret_cast<const char*>(data.getData()) + 1, "KTX ", 4) != 0)
            return false;

        RoxFormats::KhronosTexture KhronosTexture;
        const size_t header_size = KhronosTexture.decodeHeader(data.getData(), data.getSize());
        if (!header_size)
        {
            RoxLogger::log() << "unable to load KhronosTexture: invalid or unsupported KhronosTexture header in file " << name << "\n";
            return false;
        }

        RoxRender::RoxTexture::COLOR_FORMAT cf;

        switch (KhronosTexture.pf)
        {
        case RoxFormats::KhronosTexture::RGB: cf = RoxRender::RoxTexture::COLOR_RGB; break;
        case RoxFormats::KhronosTexture::RGBA: cf = RoxRender::RoxTexture::COLOR_RGBA; break;
        case RoxFormats::KhronosTexture::BGRA: cf = RoxRender::RoxTexture::COLOR_BGRA; break;

        case RoxFormats::KhronosTexture::ETC1: cf = RoxRender::RoxTexture::ETC1; break;
        case RoxFormats::KhronosTexture::ETC2: cf = RoxRender::RoxTexture::ETC2; break;
        case RoxFormats::KhronosTexture::ETC2_EAC: cf = RoxRender::RoxTexture::ETC2_EAC; break;
        case RoxFormats::KhronosTexture::ETC2_A1: cf = RoxRender::RoxTexture::ETC2_A1; break;

        case RoxFormats::KhronosTexture::PVR_RGBA2B: cf = RoxRender::RoxTexture::PVR_RGB2B; break;
        case RoxFormats::KhronosTexture::PVR_RGB4B: cf = RoxRender::RoxTexture::PVR_RGBA4B; break;
        case RoxFormats::KhronosTexture::PVR_RGB2B: cf = RoxRender::RoxTexture::PVR_RGBA2B; break;
        case RoxFormats::KhronosTexture::PVR_RGBA4B: cf = RoxRender::RoxTexture::PVR_RGBA4B; break;

        default: RoxLogger::log() << "unable to load KhronosTexture: unsupported color format in file " << name << "\n"; return false;
        }

        const int mip_off = m_load_ktx_mip_offset >= static_cast<int>(KhronosTexture.mipmap_count) ? 0 : m_load_ktx_mip_offset;
        char* d = reinterpret_cast<char*>(KhronosTexture.data);
        RoxMemory::RoxMemoryReader r(KhronosTexture.data, KhronosTexture.data_size);
        for (unsigned int i = 0; i < KhronosTexture.mipmap_count; ++i)
        {
            const unsigned int size = r.read<unsigned int>();
            if (static_cast<int>(i) >= mip_off)
            {
                if (r.getRemained() < size)
                {
                    RoxLogger::log() << "unable to load KhronosTexture: invalid RoxTexture mipmap size in file " << name << "\n";
                    return false;
                }

                memmove(d, r.getData(), size);
                d += size;
            }
            r.skip(size);
        }

        const int width = KhronosTexture.width >> mip_off;
        const int height = KhronosTexture.height >> mip_off;
        readMeta(res, data);
        return res.tex.buildTexture(KhronosTexture.data, width > 0 ? width : 1, height > 0 ? height : 1, cf, KhronosTexture.mipmap_count - mip_off);
    }

    bool RoxTexture::m_load_dds_flip = false;
    int RoxTexture::m_load_dds_mip_offset = 0;

    bool RoxTexture::loadDds(SharedTexture& res, RoxResources::RoxResourceData& data, const char* name)
    {
        if (!data.getSize())
            return false;

        if (data.getSize() < 4)
            return false;

        if (memcmp(data.getData(), "DDS ", 4) != 0)
            return false;

        RoxFormats::DirectDrawSurface DirectDrawSurface;
        const size_t header_size = DirectDrawSurface.decodeHeader(data.getData(), data.getSize());
        if (!header_size)
        {
            RoxLogger::log() << "unable to load DirectDrawSurface: invalid or unsupported DirectDrawSurface header in file " << name << "\n";
            return false;
        }

        if (DirectDrawSurface.pf != RoxFormats::DirectDrawSurface::PALETTE8_RGBA && DirectDrawSurface.pf != RoxFormats::DirectDrawSurface::palette4_rgba) // ToDo
        {
            for (int i = 0; i < m_load_dds_mip_offset && DirectDrawSurface.mipmap_count > 1; ++i)
            {
                DirectDrawSurface.data = reinterpret_cast<char*>(DirectDrawSurface.data) + DirectDrawSurface.get_mip_size(0);
                if (DirectDrawSurface.width > 1)
                    DirectDrawSurface.width /= 2;
                if (DirectDrawSurface.height > 1)
                    DirectDrawSurface.height /= 2;
                --DirectDrawSurface.mipmap_count;
            }
        }

        RoxMemory::RoxTmpBufferRef tmpBuf;

        int mipmap_count = DirectDrawSurface.need_generate_mipmaps ? -1 : DirectDrawSurface.mipmap_count;
        RoxRender::RoxTexture::COLOR_FORMAT cf;
        switch (DirectDrawSurface.pf)
        {
        case RoxFormats::DirectDrawSurface::DXT1: cf = RoxRender::RoxTexture::DXT1; break;
        case RoxFormats::DirectDrawSurface::DXT2:
        case RoxFormats::DirectDrawSurface::DXT3: cf = RoxRender::RoxTexture::DXT3; break;
        case RoxFormats::DirectDrawSurface::DXT4:
        case RoxFormats::DirectDrawSurface::DXT5: cf = RoxRender::RoxTexture::DXT5; break;

        case RoxFormats::DirectDrawSurface::BGRA: cf = RoxRender::RoxTexture::COLOR_BGRA; break;
        case RoxFormats::DirectDrawSurface::RGBA: cf = RoxRender::RoxTexture::COLOR_RGBA; break;
        case RoxFormats::DirectDrawSurface::RGB: cf = RoxRender::RoxTexture::COLOR_RGB; break;
        case RoxFormats::DirectDrawSurface::GREYSCALE: cf = RoxRender::RoxTexture::GREYSCALE; break;

        case RoxFormats::DirectDrawSurface::BGR:
        {
            RoxRender::bitmapRgbToBgr(reinterpret_cast<unsigned char*>(DirectDrawSurface.data), DirectDrawSurface.width, DirectDrawSurface.height, 3);
        	cf = RoxRender::RoxTexture::COLOR_RGB;
        }
        break;

        case RoxFormats::DirectDrawSurface::PALETTE8_RGBA:
        {
            if (DirectDrawSurface.mipmap_count != 1 || DirectDrawSurface.type != RoxFormats::DirectDrawSurface::TEXTURE_2D) // ToDo
            {
                RoxLogger::log() << "unable to load DirectDrawSurface: incomplete PALETTE8_RGBA support, unable to load file " << name << "\n";
                return false;
            }

            cf = RoxRender::RoxTexture::COLOR_RGBA;
            DirectDrawSurface.data_size = DirectDrawSurface.width * DirectDrawSurface.height * 4;
            tmpBuf.allocate(DirectDrawSurface.data_size);
            DirectDrawSurface.decodePalette8Rgba(tmpBuf.getData());
            DirectDrawSurface.data = tmpBuf.getData();
            DirectDrawSurface.pf = RoxFormats::DirectDrawSurface::BGRA;
        }
        break;

        default: RoxLogger::log() << "unable to load DirectDrawSurface: unsupported color format in file " << name << "\n"; return false;
        }

        bool result = false;

        const bool decodeDxt = cf >= RoxRender::RoxTexture::DXT1 && (!RoxRender::RoxTexture::is_dxt_supported() || DirectDrawSurface.height % 2 > 0);

        switch (DirectDrawSurface.type)
        {
        case RoxFormats::DirectDrawSurface::TEXTURE_2D:
        {
            if (decodeDxt)
            {
                tmpBuf.allocate(DirectDrawSurface.getDecodedSize());
                DirectDrawSurface.decodeDxt(tmpBuf.getData());
                DirectDrawSurface.data_size = tmpBuf.getSize();
                DirectDrawSurface.data = tmpBuf.getData();
                cf = RoxRender::RoxTexture::COLOR_RGBA;
                DirectDrawSurface.pf = RoxFormats::DirectDrawSurface::BGRA;
                if (mipmap_count > 1)
                    mipmap_count = -1;
            }

            if (m_load_dds_flip)
            {
                RoxMemory::RoxTmpBufferScoped tmp_data(DirectDrawSurface.data_size);
                DirectDrawSurface.flipVertical(DirectDrawSurface.data, tmp_data.getData());
                result = res.tex.buildTexture(tmp_data.getData(), DirectDrawSurface.width, DirectDrawSurface.height, cf, mipmap_count);
            }
            else
                result = res.tex.buildTexture(DirectDrawSurface.data, DirectDrawSurface.width, DirectDrawSurface.height, cf, mipmap_count);
        }
        break;

        case RoxFormats::DirectDrawSurface::TEXTURE_CUBE:
        {
            if (decodeDxt)
            {
                tmpBuf.allocate(DirectDrawSurface.getDecodedSize());
                DirectDrawSurface.decodeDxt(tmpBuf.getData());
                DirectDrawSurface.data_size = tmpBuf.getSize();
                DirectDrawSurface.data = tmpBuf.getData();
                cf = RoxRender::RoxTexture::COLOR_RGBA;
                DirectDrawSurface.pf = RoxFormats::DirectDrawSurface::BGRA;
                if (mipmap_count > 1)
                    mipmap_count = -1;
            }

            const void* data[6];
            for (int i = 0; i < 6; ++i)
                data[i] = (const char*)DirectDrawSurface.data + i * DirectDrawSurface.data_size / 6;
            result = res.tex.buildCubemap(data, DirectDrawSurface.width, DirectDrawSurface.height, cf, mipmap_count);
        }
        break;

        default:
        {
            RoxLogger::log() << "unable to load DirectDrawSurface: unsupported RoxTexture type in file " << name << "\n";
            tmpBuf.free();
            return false;
        }
        }

        tmpBuf.free();
        read_meta(res, data);
        return result;
    }

    bool RoxRender::load_tga(SharedTexture& res, RoxResources::RoxResourceData& data, const char* name)
    {
        if (!data.getSize())
            return false;

        RoxFormats::RTga tga;
        const size_t header_size = tga.decodeHeader(data.getData(), data.getSize());
        if (!header_size)
            return false;

        RoxRender::RoxTexture::COLOR_FORMAT color_format;
        switch (tga.channels)
        {
        case 4: color_format = RoxRender::RoxTexture::COLOR_BGRA; break;
        case 3: color_format = RoxRender::RoxTexture::COLOR_RGB; break;
        case 1: color_format = RoxRender::RoxTexture::GREYSCALE; break;
        default: RoxLogger::log() << "unable to load tga: unsupported color format in file " << name << "\n"; return false;
        }

        typedef unsigned char uchar;

        RoxMemory::RoxTmpBufferRef  tmp_data;
        const void* color_data = tga.data;
        if (tga.rle)
        {
            tmp_data.allocate(tga.uncompressedSize );
            if (!tga.decode_rle(tmp_data.getData()))
            {
                tmp_data.free();
                RoxLogger::log() << "unable to load tga: unable to decode rle in file " << name << "\n";
                return false;
            }

            color_data = tmp_data.getData();
        }
        else if (header_size + tga.uncompressedSize > data.getSize())
        {
            RoxLogger::log() << "unable to load tga: lack of data, probably corrupted file " << name << "\n";
            return false;
        }

        if (tga.channels == 3 || tga.horisontalFlip || tga.verticalFlip)
        {
            if (!tmp_data.getData())
            {
                tmp_data.allocate(tga.uncompressedSize );

                if (tga.horisontalFlip || tga.verticalFlip)
                {
                    if (tga.horisontalFlip)
                        tga.flipHorizontal(color_data, tmp_data.getData());

                    if (tga.verticalFlip)
                        tga.flipVertical(color_data, tmp_data.getData());
                }
                else
                    tmp_data.copyFrom(color_data, tga.uncompressedSize );

                color_data = tmp_data.getData();
            }
            else
            {
                if (tga.horisontalFlip)
                    tga.flipHorizontal(tmp_data.getData(), tmp_data.getData());

                if (tga.verticalFlip)
                    tga.flipVertical(tmp_data.getData(), tmp_data.getData());
            }

            if (tga.channels == 3)
                RoxRender::bitmapRgbToBgr((unsigned char*)color_data, tga.width, tga.height, 3);
        }

        const bool result = res.tex.buildTexture(color_data, tga.width, tga.height, color_format);
        tmp_data.free();
        read_meta(res, data);
        return result;
    }

    inline RoxRender::RoxTexture::WRAP get_wrap(const std::string& s)
    {
        if (s == "repeat")
            return RoxRender::RoxTexture::WRAP_REPEAT;
        if (s == "mirror")
            return RoxRender::RoxTexture::WRAP_REPEAT_MIRROR;

        return RoxRender::RoxTexture::WRAP_CLAMP;
    }

    bool RoxTexture::readMeta(SharedTexture & res, RoxResources::RoxResourceData& data)
    {
        RoxFormats::Meta m;
        if (!m.read(data.getData(), data.getSize()))
            return false;

        bool setWrap = false;
        RoxRender::RoxTexture::WRAP s, t;
        s = t = RoxRender::RoxTexture::WRAP_REPEAT;

        for (int i = 0; i < (int)m.values.size(); ++i)
        {
            if (m.values[i].first == "nya_wrap_s")
                s = get_wrap(m.values[i].second), setWrap = true;
            else if (m.values[i].first == "nya_wrap_t")
                t = get_wrap(m.values[i].second), setWrap = true;
            else if (m.values[i].first == "nya_aniso")
            {
                const int aniso = atoi(m.values[i].second.c_str());
                res.tex.setAniso(aniso > 0 ? aniso : 0);
            }
        }

        if (setWrap)
            res.tex.setWrap(s, t);

        return true;
    }

    bool RoxTextureInternal::set(int slot) const
    {
        if (!m_shared.isValid())
        {
            RoxRender::RoxTexture::unbind(slot);
            return false;
        }

        m_last_slot = slot;
        m_shared->tex.bind(slot);

        return true;
    }

    void RoxTextureInternal::unset() const
    {
        if (!m_shared.isValid())
            return;

        m_shared->tex.unbind(m_last_slot);
    }

    unsigned int RoxTexture::getWidth() const
    {
        if (!internal().getSharedData().isValid())
            return 0;

        return internal().getSharedData()->tex.getWidth();
    }

    unsigned int RoxTexture::getHeight() const
    {
        if (!internal().getSharedData().isValid())
            return 0;

        return internal().getSharedData()->tex.getHeight();
    }

    RoxTexture::color_format RoxTexture::get_format() const
    {
        if (!internal().getSharedData().isValid())
            return RoxRender::RoxTexture::COLOR_RGBA;

        return internal().getSharedData()->tex.get_color_format();
    }

    RoxMemory::RoxTmpBufferRef  RoxTexture::getData() const
    {
        RoxMemory::RoxTmpBufferRef  result;
        if (internal().getSharedData().isValid())
            internal().getSharedData()->tex.getData(result);
        return result;
    }

    RoxMemory::RoxTmpBufferRef  RoxTexture::getData(int x, int y, int width, int height) const
    {
        RoxMemory::RoxTmpBufferRef  result;
        if (internal().getSharedData().isValid())
            internal().getSharedData()->tex.getData(result, x, y, width, height);
        return result;
    }

    bool RoxTexture::isCubemap() const
    {
        if (!internal().getSharedData().isValid())
            return false;

        return internal().getSharedData()->tex.isCubemap();
    }

    bool RoxTexture::build(const void* data, unsigned int width, unsigned int height, color_format format)
    {
        RoxTextureInternal::SharedResources::RoxSharedResourceMutableRef ref;
        if (m_internal.m_shared.getRefCount() == 1 && !m_internal.m_shared.getName())  //was created and unique
        {
            ref = RoxTextureInternal::SharedResources::modify(m_internal.m_shared);
            return ref->tex.buildTexture(data, width, height, format);
        }

        m_internal.unload();

        ref = m_internal.getSharedResources().create();
        if (!ref.isValid())
            return false;

        m_internal.m_shared = ref;
        return ref->tex.buildTexture(data, width, height, format);
    }

    bool RoxTexture::crop(uint x, uint y, uint width, uint height)
    {
        if (!width || !height)
        {
            unload();
            return true;
        }

        if (x == 0 && y == 0 && width == getWidth() && height == getHeight())
            return true;

        if (x + width > getWidth() || y + height > getHeight())
            return false;

        if (!m_internal.m_shared.isValid())
            return false;

        RoxRender::RoxTexture new_tex;
        if (!new_tex.buildTexture(0, width, height, get_format()))
            return false;

        if (!new_tex.copyRegion(m_internal.m_shared->tex, x, y, width, height, 0, 0))
        {
            new_tex.release();

            int channels;
            switch (get_format())
            {
            case RoxRender::RoxTexture::COLOR_RGBA:
            case RoxRender::RoxTexture::COLOR_BGRA: channels = 4; break;
            case RoxRender::RoxTexture::COLOR_RGB: channels = 3; break;
            case RoxRender::RoxTexture::GREYSCALE: channels = 1; break;
            default: return false;
            }

            RoxMemory::RoxTmpBufferScoped buf(getData());
            RoxRender::bitmapCrop((unsigned char*)buf.getData(), getWidth(), getHeight(), x, y, width, height, channels);
            return build(buf.getData(), width, height, get_format());
        }

        RoxTextureInternal::SharedResources::RoxSharedResourceMutableRef ref;
        if (m_internal.m_shared.getRefCount() == 1 && !m_internal.m_shared.getName())  //was created and unique
        {
            ref = RoxTextureInternal::SharedResources::modify(m_internal.m_shared);
            ref->tex.release();
            ref->tex = new_tex;
            return true;
        }

        m_internal.unload();

        ref = m_internal.getSharedResources().create();
        if (!ref.isValid())
        {
            new_tex.release();
            return false;
        }

        ref->tex = new_tex;
        m_internal.m_shared = ref;

        return true;
    }

    inline void grey_to_color(const unsigned char* data_from, unsigned char* data_to, size_t src_size, int channels)
    {
        const unsigned char* src = data_from;
        unsigned char* dst = data_to;

        for (size_t i = 0; i < src_size; ++i, ++src)
        {
            for (int j = 0; j < channels; ++j, ++dst)
                *dst = *src;
        }
    }

    bool RoxTexture::updateRegion(const void* data, uint x, uint y, uint width, uint height, color_format format, int mip)
    {
        if (!data)
            return false;

        if (get_format() == format)
            return updateRegion(data, x, y, width, height, mip);

        switch (get_format())
        {
        case RoxRender::RoxTexture::COLOR_BGRA:
        case RoxRender::RoxTexture::COLOR_RGBA:
        {
            RoxMemory::RoxTmpBufferScoped buf(width * height * 4);

            if (format == RoxRender::RoxTexture::COLOR_BGRA || format == RoxRender::RoxTexture::COLOR_RGBA)
            {
                buf.copyFrom(data, buf.getSize());
                RoxRender::bitmapRgbToBgr((unsigned char*)buf.getData(), width, height, 4);
            }
            else if (format == RoxRender::RoxTexture::COLOR_RGB)
            {
                RoxRender::bitmapRgbToRgba((const unsigned char*)data, width, height, 255, (unsigned char*)buf.getData());
                if (get_format() == RoxRender::RoxTexture::COLOR_BGRA)
                    RoxRender::bitmapRgbToBgr((unsigned char*)buf.getData(), width, height, 4);
            }
            else if (format == RoxRender::RoxTexture::GREYSCALE)
                grey_to_color((const unsigned char*)data, (unsigned char*)buf.getData(), width * height, 4);
            else
                return false;

            return updateRegion(buf.getData(), x, y, width, height, mip);
        }
        break;

        case RoxRender::RoxTexture::COLOR_RGB:
        {
            RoxMemory::RoxTmpBufferScoped buf(width * height * 3);
            if (format == RoxRender::RoxTexture::COLOR_BGRA || format == RoxRender::RoxTexture::COLOR_RGBA)
            {
                RoxRender::bitmapRgbaToRgb((const unsigned char*)data, width, height, (unsigned char*)buf.getData());
                if (format == RoxRender::RoxTexture::COLOR_BGRA)
                    RoxRender::bitmapRgbToBgr((unsigned char*)buf.getData(), width, height, 3);
            }
            else if (format == RoxRender::RoxTexture::GREYSCALE)
                grey_to_color((const unsigned char*)data, (unsigned char*)buf.getData(), width * height, 3);
            else
                return false;

            return updateRegion(buf.getData(), x, y, width, height, mip);
        }
        break;

        //ToDo

        default: return false;
        }

        return false;
    }

    bool RoxTexture::updateRegion(const void* data, uint x, uint y, uint width, uint height, int mip)
    {
        if (!width || !height)
            return x <= getWidth() && y <= getHeight();

        RoxTextureInternal::SharedResources::RoxSharedResourceMutableRef ref;
        if (m_internal.m_shared.getRefCount() == 1 && !m_internal.m_shared.getName())  //was created and unique
        {
            ref = RoxTextureInternal::SharedResources::modify(m_internal.m_shared);
            return ref->tex.updateRegion(data, x, y, width, height);
        }

        if (!m_internal.m_shared.isValid())
            return false;

        const uint w = m_internal.m_shared->tex.getWidth();
        const uint h = m_internal.m_shared->tex.getHeight();

        if (!x && !y && w == width && h == height && mip < 0)
            return build(data, w, h, get_format());

        SharedTexture new_stex;
        if (!new_stex.tex.buildTexture(0, w, h, get_format()))
            return false;

        if (!new_stex.tex.copyRegion(m_internal.m_shared->tex, 0, 0, w, h, 0, 0))
        {
            const RoxMemory::RoxTmpBufferScoped buf(getData(0, 0, w, h));
            if (!buf.getSize() || !new_stex.tex.buildTexture(buf.getData(), w, h, get_format()))
            {
                new_stex.tex.release();
                return false;
            }
        }

        create(new_stex);
        return updateRegion(data, x, y, width, height, mip);
    }

    bool RoxTexture::updateRegion(const RoxTextureProxy& source, uint x, uint y)
    {
        if (!source.isValid())
            return false;

        return updateRegion(source, 0, 0, source->getWidth(), source->getHeight(), x, y);
    }

    bool RoxTexture::updateRegion(const RoxTexture& source, unsigned int x, unsigned int y)
    {
        return updateRegion(RoxTextureProxy(source), x, y);
    }

    bool RoxTexture::updateRegion(const RoxTextureProxy& source, uint src_x, uint src_y, uint width, uint height, uint dst_x, uint dst_y)
    {
        if (!source.isValid() || !source->internal().getSharedData().isValid())
            return false;

        if (src_x + width > source->getWidth() || src_y + height > source->getHeight())
            return false;

        if (dst_x + width > getWidth() || dst_y + height > getHeight())
            return false;

        if (!internal().getSharedData().isValid())
            return false;

        if (m_internal.m_shared.getRefCount() == 1 && !m_internal.m_shared.getName())  //was created and unique
        {
            RoxRender::RoxTexture& tex = RoxTextureInternal::SharedResources::modify(m_internal.m_shared)->tex;
            if (tex.copyRegion(source->internal().getSharedData()->tex, src_x, src_y, width, height, dst_x, dst_y))
                return true;
        }

        const RoxMemory::RoxTmpBufferScoped buf(source->getData(src_x, src_y, width, height));
        return updateRegion(buf.getData(), dst_x, dst_y, width, height, source->get_format());
    }

    bool RoxTexture::updateRegion(const RoxTexture& source, uint src_x, uint src_y, uint width, uint height, uint dst_x, uint dst_y)
    {
        return updateRegion(RoxTextureProxy(source), src_x, src_y, width, height, dst_x, dst_y);
    }

    void RoxTexture::setResourcesPrefix(const char* prefix)
    {
        RoxTextureInternal::setResourcesPrefix(prefix);
    }

    void RoxTexture::registerLoadFunction(RoxTextureInternal::LoadFunction function, bool clear_default)
    {
        RoxTextureInternal::registerLoadFunction(function, clear_default);
    }

}
