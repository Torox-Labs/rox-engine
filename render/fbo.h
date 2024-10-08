//nya-engine (C) nyan.developer@gmail.com released under the MIT license (see LICENSE)

#pragma once

#include "texture.h"
#include <vector>

namespace nya_render
{

class fbo
{
public:
    enum cubemap_side
    {
        cube_positive_x,
        cube_negative_x,
        cube_positive_y,
        cube_negative_y,
        cube_positive_z,
        cube_negative_z
    };

public:
    void set_color_target(const texture &tex,unsigned int attachment_idx=0,unsigned int samples=1);
    void set_color_target(const texture &tex,cubemap_side side,unsigned int attachment_idx=0,unsigned int samples=1);
    void set_depth_target(const texture &tex);

public:
    static unsigned int get_max_color_attachments();
    static unsigned int get_max_msaa();

public:
    void release();

public:
    void bind() const;
    static void unbind();

public:
    static const fbo get_current();

public:
    fbo(): m_fbo_idx(-1),m_width(0),m_height(0),m_samples(0),m_depth_texture(-1),m_update(false) {}

private:
    mutable int m_fbo_idx;
    unsigned int m_width,m_height,m_samples;
    std::vector<int> m_attachment_textures;
    std::vector<int> m_attachment_sides;
    int m_depth_texture;
    mutable bool m_update;
};

}
