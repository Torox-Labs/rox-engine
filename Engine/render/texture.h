//nya-engine (C) nyan.developer@gmail.com released under the MIT license (see LICENSE)

#pragma once

namespace nya_memory { class tmp_buffer_ref; }

struct ID3D11Texture2D;

namespace nya_render
{

class texture
{
    friend class fbo;

public:
    enum color_format
    {
        color_rgb,
        color_rgba,
        color_bgra,
        //color_r,
        greyscale,

        color_r32f,
        color_rgb32f,
        color_rgba32f,

        depth16,
        depth24,
        depth32,

        dxt1,
        dxt3,
        dxt5,

        dxt2=dxt3,
        dxt4=dxt5,

        etc1,
        etc2,
        etc2_eac,
        etc2_a1,

        pvr_rgb2b,
        pvr_rgb4b,
        pvr_rgba2b,
        pvr_rgba4b
    };

    static bool is_dxt_supported();

    typedef unsigned int uint;

public:
    //mip_count= -1 means "generate mipmaps". You have to provide a single mip or a complete mipmap pyramid instead
    //resulting format may vary on different platforms, allways use get_color_format() to get actual texture format
    bool build_texture(const void *data,uint width,uint height,color_format format,
                       int mip_count= -1);

	//order: positive_x,negative_x,positive_y,negative_y,positive_z,negative_z
	bool build_cubemap(const void *data[6],uint width,uint height,color_format format,
                       int mip_count= -1);
public:
    //mip= -1 means "update all mipmaps"
    bool update_region(const void *data,uint x,uint y,uint width,uint height,int mip=-1);

    bool copy_region(const texture &src,uint src_x,uint src_y,uint src_width,uint src_height,uint dst_x,uint dst_y);

public:
    void bind(uint layer) const;
    static void unbind(uint layer);

public:
    enum wrap
    {
        wrap_clamp,
        wrap_repeat,
        wrap_repeat_mirror
    };

    void set_wrap(wrap s,wrap t);

    enum filter
    {
        filter_nearest,
        filter_linear
    };

    void set_filter(filter minification,filter magnification,filter mipmap);
    void set_aniso(uint level);

    static void set_default_wrap(wrap s,wrap t);
    static void set_default_filter(filter minification,filter magnification,filter mipmap);
    static void set_default_aniso(uint level);

public:
    bool get_data(nya_memory::tmp_buffer_ref &data) const;
    bool get_data(nya_memory::tmp_buffer_ref &data,uint x,uint y,uint w,uint h) const;
    uint get_width() const;
    uint get_height() const;
    color_format get_color_format() const;
    bool is_cubemap() const;

    static void get_default_wrap(wrap &s,wrap &t);
    static void get_default_filter(filter &minification,filter &magnification,filter &mipmap);
    static uint get_default_aniso();

    static unsigned int get_max_dimension();

    static unsigned int get_format_bpp(color_format f);

public:
    void release();

public:
    static uint get_used_vmem_size();

public:
    unsigned int get_gl_tex_id() const;
    ID3D11Texture2D *get_dx11_tex_id() const;

public:
    texture(): m_tex(-1),m_width(0),m_height(0),m_is_cubemap(false),m_filter_set(false),m_aniso_set(false),
               m_aniso(0),m_filter_min(filter_linear),m_filter_mag(filter_linear),m_filter_mip(filter_linear) {}

private:
    bool build_texture(const void *data[6],bool is_cubemap,uint width,uint height,
                       color_format format,int mip_count= -1);
private:
    int m_tex;
    uint m_width,m_height;
    color_format m_format;
    bool m_is_cubemap;
    bool m_filter_set;
    bool m_aniso_set;
    unsigned int m_aniso;
    filter m_filter_min,m_filter_mag,m_filter_mip;
};

}
