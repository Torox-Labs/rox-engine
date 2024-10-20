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

#include "RoxTexture.h"
#include <vector>

namespace RoxRender
{

class RoxFbo
{
public:
    enum CUBEMAP_SIDE
    {
        CUBE_POSITIVE_X,
        CUBE_NEGATIVE_X,
        CUBE_POSITIVE_Y,
        CUBE_NEGATIVE_Y,
        CUBE_POSITIVE_Z,
        CUBE_NEGATIVE_Z
    };

public:
    void setColorTarget(const RoxTexture &tex,unsigned int attachment_idx=0,unsigned int samples=1);
    void setColorTarget(const RoxTexture &tex,CUBEMAP_SIDE side,unsigned int attachment_idx=0,unsigned int samples=1);
    void setDepthTarget(const RoxTexture &tex);

public:
    static unsigned int getMaxColorAttachments();
    static unsigned int getMaxMsaa();

public:
    void release();

public:
    void bind() const;
    static void unbind();

public:
    static const RoxFbo getCurrent();

public:
    RoxFbo(): m_fbo_idx(-1),m_width(0),m_height(0),m_samples(0),m_depth_texture(-1),m_update(false) {}

private:
    mutable int m_fbo_idx;
    unsigned int m_width,m_height,m_samples;
    std::vector<int> m_attachment_textures;
    std::vector<int> m_attachment_sides;
    int m_depth_texture;
    mutable bool m_update;
};

}
