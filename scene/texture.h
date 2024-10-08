//nya-engine (C) nyan.developer@gmail.com released under the MIT license (see LICENSE)

#pragma once

#include "shared_resources.h"
#include "render/texture.h"
#include "proxy.h"

namespace nya_scene
{

struct shared_texture
{
    nya_render::texture tex;

    bool release()
    {
        tex.release();
        return true;
    }
};

class texture_internal: public scene_shared<shared_texture>
{
    friend class texture;

public:
    bool set(int slot=0) const;
    void unset() const;

public:
    texture_internal():m_last_slot(0) {}

private:
    mutable int m_last_slot;
};

class texture;

typedef proxy<texture> texture_proxy;

class texture
{
public:
    bool load(const char *name)
    {
        texture_internal::default_load_function(load_tga);
        texture_internal::default_load_function(load_dds);
        texture_internal::default_load_function(load_ktx);
        return m_internal.load(name);
    }

    void unload() { return m_internal.unload(); }

public:
    void create(const shared_texture &res) { m_internal.create(res); }

public:
    typedef nya_render::texture::color_format color_format;

    const char *get_name() const { return internal().get_name(); }
    unsigned int get_width() const;
    unsigned int get_height() const;
    color_format get_format() const;
    nya_memory::tmp_buffer_ref get_data() const;
    nya_memory::tmp_buffer_ref get_data(int x,int y,int width,int height) const;
    bool is_cubemap() const;

public:
    typedef unsigned int uint;
    bool build(const void *data,uint width,uint height,color_format format);
    bool crop(uint x,uint y,uint width,uint height);
    bool update_region(const void *data,uint x,uint y,uint width,uint height,int mip=-1);
    bool update_region(const void *data,uint x,uint y,uint width,uint height,color_format format,int mip=-1);
    bool update_region(const texture_proxy &source,uint x,uint y);
    bool update_region(const texture &source,uint x,uint y);
    bool update_region(const texture_proxy &source,uint src_x,uint src_y,uint width,uint height,uint dst_x,uint dst_y);
    bool update_region(const texture &source,uint src_x,uint src_y,uint width,uint height,uint dst_x,uint dst_y);

public:
    texture() {}
    texture(const char *name) { *this=texture(); load(name); }

public:
    static bool load_tga(shared_texture &res,resource_data &data,const char* name);
    static bool load_dds(shared_texture &res,resource_data &data,const char* name);
    static bool load_ktx(shared_texture &res,resource_data &data,const char* name);

    static void set_load_dds_flip(bool flip) { m_load_dds_flip=flip; }
    static void set_dds_mip_offset(int off) { m_load_dds_mip_offset=off; }
    static void set_ktx_mip_offset(int off) { m_load_ktx_mip_offset=off; }

    static bool read_meta(shared_texture &res,resource_data &data);

public:
    static void set_resources_prefix(const char *prefix);
    static void register_load_function(texture_internal::load_function function,bool clear_default=true);

public:
    const texture_internal &internal() const { return m_internal; }

private:
    texture_internal m_internal;
    static bool m_load_dds_flip;
    static int m_load_dds_mip_offset;
    static int m_load_ktx_mip_offset;
};

}
