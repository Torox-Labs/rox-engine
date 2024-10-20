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

#include "RoxFbo.h"
#include "RoxRenderApi.h"

namespace RoxRender
{

void RoxFbo::setColorTarget(const RoxTexture &tex,CUBEMAP_SIDE side,unsigned int attachment_idx,unsigned int samples)
{
    if(attachment_idx>=getMaxColorAttachments())
        return;

    if(samples>getMaxMsaa())
        samples=getMaxMsaa();

    if(attachment_idx>=(int)m_attachment_textures.size())
    {
        m_attachment_textures.resize(attachment_idx+1,-1);
        m_attachment_sides.resize(attachment_idx+1,-1);
    }

    m_attachment_textures[attachment_idx]=tex.m_tex;
    m_attachment_sides[attachment_idx]=side;
    m_samples=samples;
    if(tex.m_tex>=0)
        m_width=tex.m_width,m_height=tex.m_height;
    m_update=true;
}

void RoxFbo::setColorTarget(const RoxTexture &tex,unsigned int attachment_idx,unsigned int samples)
{
    setColorTarget(tex,CUBEMAP_SIDE(-1),attachment_idx,samples);
}

void RoxFbo::setDepthTarget(const RoxTexture &tex)
{
    m_depth_texture=tex.m_tex;
    if(tex.m_tex>=0)
        m_width=tex.m_width,m_height=tex.m_height;
    m_update=true;
}

void RoxFbo::release()
{
	if(m_fbo_idx<0)
		return;

    getApiInterface().removeTarget(m_fbo_idx);
    if(getApiState().target==m_fbo_idx)
        getApiState().target=-1;

    *this=RoxFbo();
}

void RoxFbo::bind() const
{
    if(getApiState().target>=0 && getApiState().target!=m_fbo_idx)
        getApiInterface().resolveTarget(getApiState().target);

    if(m_update)
    {
        const int idx=getApiInterface().createTarget(m_width,m_height,m_samples,m_attachment_textures.data(),
                                                        m_attachment_sides.data(),(int)m_attachment_textures.size(),m_depth_texture);
        if(m_fbo_idx>=0)
        {
            if(getApiState().target==m_fbo_idx)
            {
                getApiInterface().resolveTarget(m_fbo_idx);
                getApiState().target=idx;
            }
            getApiInterface().removeTarget(m_fbo_idx);
        }
        m_fbo_idx=idx;
        m_update=false;
    }

    getApiState().target=m_fbo_idx;
}

void RoxFbo::unbind()
{
    if(getApiState().target>=0)
        getApiInterface().resolveTarget(getApiState().target);

    getApiState().target=-1;
}

const RoxFbo RoxFbo::getCurrent() { RoxFbo f; f.m_fbo_idx=getApiState().target; return f; }

unsigned int RoxFbo::getMaxColorAttachments() { return getApiInterface().getMaxTargetAttachments(); }
unsigned int RoxFbo::getMaxMsaa() { return getApiInterface().getMaxTargetMsaa(); }

}
