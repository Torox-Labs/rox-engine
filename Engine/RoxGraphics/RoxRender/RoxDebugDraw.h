// Updated By the ROX_ENGINE
// Copyright (C) 2024 Torox Project
// Portions Copyright (C) 2013 nyan.developer@gmail.com (nya-engine)
//
// This file was modified by the Torox Project.
// Update the render api intefrace to check Metal 1th.
//
// This file incorporates code from the nya-engine project, which is licensed under the MIT License.
// See the LICENSE-MIT file in the root directory for more information.
//
// This file is also part of the Rox-engine, which is licensed under a dual-license system:
// 1. Free Use License (for non-commercial and commercial use under specific conditions)
// 2. Commercial License (for use on proprietary platforms)
// See the LICENSE file in the root directory for the full Rox-engine license terms.

#pragma once

#include "RoxVbo.h"
#include "RoxShader.h"
#include "RoxSkeleton.h"
#include "RoxMath/RoxFrustum.h"

namespace RoxRender
{

class RoxDebugDraw
{
public:
    void clear() { m_line_verts.clear(); m_point_verts.clear(); m_tri_verts.clear(); }
    void addPoint(const RoxMath::Vector3 &pos,
                   const RoxMath::Vector4 &color=RoxMath::Vector4(1.0f,1.0f,1.0f,1.0f));
    void addLine(const RoxMath::Vector3 &pos,const RoxMath::Vector3 &pos2,
                  const RoxMath::Vector4 &color=RoxMath::Vector4(1.0f,1.0f,1.0f,1.0f));
    void addLine(const RoxMath::Vector3 &pos,const RoxMath::Vector3 &pos2,
                  const RoxMath::Vector4 &color,const RoxMath::Vector4 &color2);
    void addTri(const RoxMath::Vector3 &pos, const RoxMath::Vector3 &pos2, const RoxMath::Vector3 &pos3,
                 const RoxMath::Vector4 &color=RoxMath::Vector4(1.0f,1.0f,1.0f,1.0f));
    void addQuad(const RoxMath::Vector3 &pos, const RoxMath::Vector3 &pos2, const RoxMath::Vector3 &pos3,
                  const RoxMath::Vector3 &pos4, const RoxMath::Vector4 &color=RoxMath::Vector4(1.0f,1.0f,1.0f,1.0f));
    void addSkeleton(const RoxSkeleton &sk,
                      const RoxMath::Vector4 &color=RoxMath::Vector4(1.0f,1.0f,1.0f,1.0f));
    void addAabb(const RoxMath::Aabb &box,
                  const RoxMath::Vector4 &color=RoxMath::Vector4(1.0f,1.0f,1.0f,1.0f));

public:
    void setPointSize(float size) { m_point_size=size; }
    void setLineWidth(float width) { m_line_width=width; }

public:
    void draw() const;

public:
    void release();

public:
    RoxDebugDraw(): m_initialised(false),m_point_size(1.0f),m_line_width(1.0f) {}

private:
    mutable RoxVBO m_vbo;
    mutable RoxShader m_shader;

    struct vert { RoxMath::Vector3 pos; RoxMath::Vector4 color; };
    std::vector<vert> m_line_verts;
    std::vector<vert> m_point_verts;
    std::vector<vert> m_tri_verts;

    mutable bool m_initialised;
    float m_point_size;
    float m_line_width;
};

}
