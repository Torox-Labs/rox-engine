// Updated By the ROX_ENGINE
// Copyright (C) 2024 Torox Project
// Portions Copyright (C) 2013 nyan.developer@gmail.com (nya-engine)
//
// This file was modified by the Torox Project.
// 
// This file incorporates code from the nya-engine project, which is licensed under the MIT License.
// See the LICENSE-MIT file in the root directory for more information.
//
// This file is also part of the Rox-engine, which is licensed under a dual-license system:
// 1. Free Use License (for non-commercial and commercial use under specific conditions)
// 2. Commercial License (for use on proprietary platforms)
// See the LICENSE file in the root directory for the full Rox-engine license terms.

#include "RoxDebugDraw.h"

namespace RoxRender
{

void RoxDebugDraw::addPoint(const RoxMath::Vector3 &pos,const RoxMath::Vector4 &color)
{
    Vertex v; v.pos=pos; v.color=color; m_point_vertices.push_back(v);
}

void RoxDebugDraw::addLine(const RoxMath::Vector3 &pos,const RoxMath::Vector3 &pos2,const RoxMath::Vector4 &color)
{
    addLine(pos,pos2,color,color);
}

void RoxDebugDraw::addLine(const RoxMath::Vector3 &pos,const RoxMath::Vector3 &pos2,
                          const RoxMath::Vector4 &color,const RoxMath::Vector4 &color2)
{
    Vertex v;
    v.pos=pos, v.color=color; m_line_vertices.push_back(v);
    v.pos=pos2, v.color=color2; m_line_vertices.push_back(v);
}

void RoxDebugDraw::addTri(const RoxMath::Vector3 &pos,const RoxMath::Vector3 &pos2,
                         const RoxMath::Vector3 &pos3,const RoxMath::Vector4 &color)
{
    Vertex v;
    v.color=color;
    v.pos=pos; m_tri_vertices.push_back(v);
    v.pos=pos2; m_tri_vertices.push_back(v);
    v.pos=pos3; m_tri_vertices.push_back(v);
}

void RoxDebugDraw::addQuad(const RoxMath::Vector3 &pos,const RoxMath::Vector3 &pos2,const RoxMath::Vector3 &pos3,
                          const RoxMath::Vector3 &pos4,const RoxMath::Vector4 &color)
{
    addTri(pos,pos2,pos3,color);
    addTri(pos,pos3,pos4,color);
}

void RoxDebugDraw::addSkeleton(const RoxSkeleton &sk,const RoxMath::Vector4 &color)
{
    for(int i=0;i<sk.getBonesCount();++i)
    {
        const RoxMath::Vector3 pos=sk.getBonePos(i);
        addPoint(pos,color);

        const int parent=sk.getBoneParentIdx(i);
        if(parent<0)
            continue;

        addLine(pos,sk.getBonePos(parent),color);
    }
}

void RoxDebugDraw::addAabb(const RoxMath::Aabb &box,const RoxMath::Vector4 &color)
{
    const RoxMath::Vector3 &o=box.origin;
    const RoxMath::Vector3 &d=box.delta;

    RoxMath::Vector3 p[8]={o,o,o,o,o,o,o,o};
    p[0]+=d;
    p[1].x-=d.x; p[1].y+=d.y; p[1].z+=d.z;
    p[2].x-=d.x; p[2].y+=d.y; p[2].z-=d.z;
    p[3].x+=d.x; p[3].y+=d.y; p[3].z-=d.z;

    p[4].x-=d.x; p[4].y-=d.y; p[4].z+=d.z;
    p[5].x+=d.x; p[5].y-=d.y; p[5].z+=d.z;
    p[6].x+=d.x; p[6].y-=d.y; p[6].z-=d.z;
    p[7]-=d;
    
    addLine(p[0],p[1],color);
    addLine(p[1],p[2],color);
    addLine(p[2],p[3],color);
    addLine(p[3],p[0],color);
       
    addLine(p[4],p[5],color);
    addLine(p[5],p[6],color);
    addLine(p[6],p[7],color);
    addLine(p[7],p[4],color);
       
    addLine(p[0],p[5],color);
    addLine(p[1],p[4],color);
    addLine(p[2],p[7],color);
    addLine(p[3],p[6],color);
}

void RoxDebugDraw::draw() const
{
    if(!m_initialised)
    {
        m_vbo.setColors(sizeof(float)*3,4);
        m_shader.addProgram(RoxShader::VERTEX,"varying vec4 color; void main() { color=gl_Color;"
                                           "gl_Position=gl_ModelViewProjectionMatrix*gl_Vertex; }");
        m_shader.addProgram(RoxShader::PIXEL,"varying vec4 color; void main() { gl_FragColor=color; }");

        m_initialised=true;
    }

    m_shader.bind();

    if(!m_point_vertices.empty())
    {
//#ifndef OPENGL_ES
        //glPointSize(m_point_size);
//#endif
        m_vbo.setElementType(RoxVBO::POINTS);
        m_vbo.setVertexData(&m_point_vertices[0],sizeof(Vertex),int(m_point_vertices.size()));
        m_vbo.bind();
        m_vbo.draw();
        m_vbo.unbind();
    }

    if(!m_line_vertices.empty())
    {
        //glLineWidth(m_line_width);
        m_vbo.setElementType(RoxVBO::LINES);
        m_vbo.setVertexData(&m_line_vertices[0],sizeof(Vertex),int(m_line_vertices.size()));
        m_vbo.bind();
        m_vbo.draw();
        m_vbo.unbind();
    }

    if(!m_tri_vertices.empty())
    {
        m_vbo.setElementType(RoxVBO::TRIANGLES);
        m_vbo.setVertexData(&m_tri_vertices[0],sizeof(Vertex),int(m_tri_vertices.size()));
        m_vbo.bind();
        m_vbo.draw();
        m_vbo.unbind();
    }

    m_shader.unbind();
}

void RoxDebugDraw::release()
{
    m_vbo.release();
    m_shader.release();
}

}
