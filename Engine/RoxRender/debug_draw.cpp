//nya-engine (C) nyan.developer@gmail.com released under the MIT license (see LICENSE)

#include "debug_draw.h"

namespace nya_render
{

void debug_draw::add_point(const nya_math::vec3 &pos,const nya_math::vec4 &color)
{
    vert v; v.pos=pos; v.color=color; m_point_verts.push_back(v);
}

void debug_draw::add_line(const nya_math::vec3 &pos,const nya_math::vec3 &pos2,const nya_math::vec4 &color)
{
    add_line(pos,pos2,color,color);
}

void debug_draw::add_line(const nya_math::vec3 &pos,const nya_math::vec3 &pos2,
                          const nya_math::vec4 &color,const nya_math::vec4 &color2)
{
    vert v;
    v.pos=pos, v.color=color; m_line_verts.push_back(v);
    v.pos=pos2, v.color=color2; m_line_verts.push_back(v);
}

void debug_draw::add_tri(const nya_math::vec3 &pos,const nya_math::vec3 &pos2,
                         const nya_math::vec3 &pos3,const nya_math::vec4 &color)
{
    vert v;
    v.color=color;
    v.pos=pos; m_tri_verts.push_back(v);
    v.pos=pos2; m_tri_verts.push_back(v);
    v.pos=pos3; m_tri_verts.push_back(v);
}

void debug_draw::add_quad(const nya_math::vec3 &pos,const nya_math::vec3 &pos2,const nya_math::vec3 &pos3,
                          const nya_math::vec3 &pos4,const nya_math::vec4 &color)
{
    add_tri(pos,pos2,pos3,color);
    add_tri(pos,pos3,pos4,color);
}

void debug_draw::add_skeleton(const skeleton &sk,const nya_math::vec4 &color)
{
    for(int i=0;i<sk.get_bones_count();++i)
    {
        const nya_math::vec3 pos=sk.get_bone_pos(i);
        add_point(pos,color);

        const int parent=sk.get_bone_parent_idx(i);
        if(parent<0)
            continue;

        add_line(pos,sk.get_bone_pos(parent),color);
    }
}

void debug_draw::add_aabb(const nya_math::aabb &box,const nya_math::vec4 &color)
{
    const nya_math::vec3 &o=box.origin;
    const nya_math::vec3 &d=box.delta;

    nya_math::vec3 p[8]={o,o,o,o,o,o,o,o};
    p[0]+=d;
    p[1].x-=d.x; p[1].y+=d.y; p[1].z+=d.z;
    p[2].x-=d.x; p[2].y+=d.y; p[2].z-=d.z;
    p[3].x+=d.x; p[3].y+=d.y; p[3].z-=d.z;

    p[4].x-=d.x; p[4].y-=d.y; p[4].z+=d.z;
    p[5].x+=d.x; p[5].y-=d.y; p[5].z+=d.z;
    p[6].x+=d.x; p[6].y-=d.y; p[6].z-=d.z;
    p[7]-=d;
    
    add_line(p[0],p[1],color);
    add_line(p[1],p[2],color);
    add_line(p[2],p[3],color);
    add_line(p[3],p[0],color);
    
    add_line(p[4],p[5],color);
    add_line(p[5],p[6],color);
    add_line(p[6],p[7],color);
    add_line(p[7],p[4],color);

    add_line(p[0],p[5],color);
    add_line(p[1],p[4],color);
    add_line(p[2],p[7],color);
    add_line(p[3],p[6],color);
}

void debug_draw::draw() const
{
    if(!m_initialised)
    {
        m_vbo.set_colors(sizeof(float)*3,4);
        m_shader.add_program(shader::vertex,"varying vec4 color; void main() { color=gl_Color;"
                                           "gl_Position=gl_ModelViewProjectionMatrix*gl_Vertex; }");
        m_shader.add_program(shader::pixel,"varying vec4 color; void main() { gl_FragColor=color; }");

        m_initialised=true;
    }

    m_shader.bind();

    if(!m_point_verts.empty())
    {
//#ifndef OPENGL_ES
        //glPointSize(m_point_size);
//#endif
        m_vbo.set_element_type(vbo::points);
        m_vbo.set_vertex_data(&m_point_verts[0],sizeof(vert),int(m_point_verts.size()));
        m_vbo.bind();
        m_vbo.draw();
        m_vbo.unbind();
    }

    if(!m_line_verts.empty())
    {
        //glLineWidth(m_line_width);
        m_vbo.set_element_type(vbo::lines);
        m_vbo.set_vertex_data(&m_line_verts[0],sizeof(vert),int(m_line_verts.size()));
        m_vbo.bind();
        m_vbo.draw();
        m_vbo.unbind();
    }

    if(!m_tri_verts.empty())
    {
        m_vbo.set_element_type(vbo::triangles);
        m_vbo.set_vertex_data(&m_tri_verts[0],sizeof(vert),int(m_tri_verts.size()));
        m_vbo.bind();
        m_vbo.draw();
        m_vbo.unbind();
    }

    m_shader.unbind();
}

void debug_draw::release()
{
    m_vbo.release();
    m_shader.release();
}

}
