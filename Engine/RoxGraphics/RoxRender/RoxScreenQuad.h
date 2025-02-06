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

#pragma once

#include "RoxVBO.h"
#include "RoxRender.h"

namespace RoxRender
{

class RoxScreenQuad
{
public:
    void init()
    {
        struct { float x,y,s,t; } verts[4];
        for(int i=0;i<4;++i)
        {
            verts[i].x=i>1?-1.0f:1.0f,verts[i].y=i%2?1.0f:-1.0f;
            verts[i].s=i>1? 0.0f:1.0f,verts[i].t=i%2?1.0f:0.0f;
        }

        m_mesh.setVertexData(verts,sizeof(verts[0]),4);
        m_mesh.setVertices(0,2);
        m_mesh.setTc(0,2*4,2);
    }

    void draw(unsigned int instances_count=1) const
    {
        m_mesh.bind();
        RoxVBO::draw(0,m_mesh.getVertsCount(), RoxVBO::TRIANGLE_STRIP,instances_count);
        m_mesh.unbind();
    }

    bool isValid() const { return m_mesh.getVertsCount()>0; }

    void release() { m_mesh.release(); }

private:
    RoxVBO m_mesh;
};

}
