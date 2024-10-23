// nya-engine (C) nyan.developer@gmail.com released under the MIT license (see LICENSE)

#pragma once

#include "RoxSharedResources.h"
#include "RoxRender/RoxTexture.h"
#include "RoxProxy.h"

namespace RoxScene
{

    struct SharedTexture
    {
        RoxRender::RoxTexture tex;

        bool release()
        {
            tex.release();
            return true;
        }
    };

    class RoxTextureInternal : public RoxSceneShared<SharedTexture>
    {
        friend class RoxTexture;

    public:
        bool set(int slot = 0) const;
        void unset() const;

    public:
        RoxTextureInternal() : m_last_slot(0) {}

    private:
        mutable int m_last_slot;
    };

    class RoxTexture;

    typedef RoxProxy<RoxTexture> RoxTextureProxy;

    class RoxTexture
    {
    public:
        bool load(const char* name)
        {
            RoxTextureInternal::defaultLoadFunction(loadTga);
            RoxTextureInternal::defaultLoadFunction(loadDds);
            RoxTextureInternal::defaultLoadFunction(loadKtx);
            return m_internal.load(name);
        }

        void unload() { return m_internal.unload(); }

    public:
        void create(const SharedTexture& res) { m_internal.create(res); }

    public:
        typedef RoxRender::RoxTexture::COLOR_FORMAT ColorFormat;

        const char* getName() const { return internal().getName(); }
        unsigned int getWidth() const;
        unsigned int getHeight() const;
        ColorFormat getFormat() const;
        RoxMemory::RoxTmpBufferRef getData() const;
        RoxMemory::RoxTmpBufferRef getData(int x, int y, int width, int height) const;
        bool isCubemap() const;

    public:
        typedef unsigned int uint;
        bool build(const void* data, uint width, uint height, ColorFormat format);
        bool crop(uint x, uint y, uint width, uint height);
        bool updateRegion(const void* data, uint x, uint y, uint width, uint height, int mip = -1);
        bool updateRegion(const void* data, uint x, uint y, uint width, uint height, ColorFormat format, int mip = -1);
        bool updateRegion(const RoxTextureProxy& source, uint x, uint y);
        bool updateRegion(const RoxTexture& source, uint x, uint y);
        bool updateRegion(const RoxTextureProxy& source, uint src_x, uint src_y, uint width, uint height, uint dst_x, uint dst_y);
        bool updateRegion(const RoxTexture& source, uint src_x, uint src_y, uint width, uint height, uint dst_x, uint dst_y);

    public:
        RoxTexture() {}
        RoxTexture(const char* name) { *this = RoxTexture(); load(name); }

    public:
        static bool loadTga(SharedTexture& res, RoxResources::RoxResourceData& data, const char* name);
        static bool loadDds(SharedTexture& res, RoxResources::RoxResourceData& data, const char* name);
        static bool loadKtx(SharedTexture& res, RoxResources::RoxResourceData& data, const char* name);

        static void setLoadDdsFlip(bool flip) { m_load_dds_flip = flip; }
        static void setDdsMipOffset(int off) { m_load_dds_mip_offset = off; }
        static void setKtxMipOffset(int off) { m_load_ktx_mip_offset = off; }

        static bool readMeta(SharedTexture& res, RoxResources::RoxResourceData& data);

    public:
        static void setResourcesPrefix(const char* prefix);
        static void registerLoadFunction(RoxTextureInternal::LoadFunction function, bool clear_default = true);

    public:
        const RoxTextureInternal& internal() const { return m_internal; }

    private:
        RoxTextureInternal m_internal;
        static bool m_load_dds_flip;
        static int m_load_dds_mip_offset;
        static int m_load_ktx_mip_offset;
    };

}
