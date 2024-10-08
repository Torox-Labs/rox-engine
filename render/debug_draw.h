//nya-engine (C) nyan.developer@gmail.com released under the MIT license (see LICENSE)

#pragma once

#include "vbo.h"
#include "shader.h"
#include "skeleton.h"
#include "math/frustum.h"

namespace nya_render
{

class debug_draw
{
public:
    void clear() { m_line_verts.clear(); m_point_verts.clear(); m_tri_verts.clear(); }
    void add_point(const nya_math::vec3 &pos,
                   const nya_math::vec4 &color=nya_math::vec4(1.0f,1.0f,1.0f,1.0f));
    void add_line(const nya_math::vec3 &pos,const nya_math::vec3 &pos2,
                  const nya_math::vec4 &color=nya_math::vec4(1.0f,1.0f,1.0f,1.0f));
    void add_line(const nya_math::vec3 &pos,const nya_math::vec3 &pos2,
                  const nya_math::vec4 &color,const nya_math::vec4 &color2);
    void add_tri(const nya_math::vec3 &pos, const nya_math::vec3 &pos2, const nya_math::vec3 &pos3,
                 const nya_math::vec4 &color=nya_math::vec4(1.0f,1.0f,1.0f,1.0f));
    void add_quad(const nya_math::vec3 &pos, const nya_math::vec3 &pos2, const nya_math::vec3 &pos3,
                  const nya_math::vec3 &pos4, const nya_math::vec4 &color=nya_math::vec4(1.0f,1.0f,1.0f,1.0f));
    void add_skeleton(const skeleton &sk,
                      const nya_math::vec4 &color=nya_math::vec4(1.0f,1.0f,1.0f,1.0f));
    void add_aabb(const nya_math::aabb &box,
                  const nya_math::vec4 &color=nya_math::vec4(1.0f,1.0f,1.0f,1.0f));

public:
    void set_point_size(float size) { m_point_size=size; }
    void set_line_width(float width) { m_line_width=width; }

public:
    void draw() const;

public:
    void release();

public:
    debug_draw(): m_initialised(false),m_point_size(1.0f),m_line_width(1.0f) {}

private:
    mutable vbo m_vbo;
    mutable shader m_shader;

    struct vert { nya_math::vec3 pos; nya_math::vec4 color; };
    std::vector<vert> m_line_verts;
    std::vector<vert> m_point_verts;
    std::vector<vert> m_tri_verts;

    mutable bool m_initialised;
    float m_point_size;
    float m_line_width;
};

}
