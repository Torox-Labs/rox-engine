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

#include "RoxFBO.h"
#include "IRoxRenderApi.h"

namespace RoxRender
{

void RoxFBO::setColorTarget(const RoxTexture &tex,CUBEMAP_SIDE side, uint attachment_idx, uint samples)
{
    if(attachment_idx>=getMaxColorAttachments())
        return;

    if(samples>getMaxMSAA())
        samples=getMaxMSAA();

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

void RoxFBO::setColorTarget(const RoxTexture &tex, uint attachment_idx, uint samples)
{
    setColorTarget(tex,CUBEMAP_SIDE(-1),attachment_idx,samples);
}

void RoxFBO::setDepthTarget(const RoxTexture& tex)
{
    m_depth_texture=tex.m_tex;
    if(tex.m_tex>=0)
        m_width=tex.m_width,m_height=tex.m_height;
    m_update=true;
}

void RoxFBO::release()
{
	if(m_fbo_idx<0)
		return;

    getAPIInterface().removeTarget(m_fbo_idx);
    if(getAPIState().target==m_fbo_idx)
        getAPIState().target=-1;

    *this= RoxFBO();
}

void RoxFBO::bind() const
{
    if(getAPIState().target>=0 && getAPIState().target!=m_fbo_idx)
        getAPIInterface().resolveTarget(getAPIState().target);

    if(m_update)
    {
        const int idx=getAPIInterface().createTarget(m_width,m_height,m_samples,m_attachment_textures.data(),
                                                        m_attachment_sides.data(),(int)m_attachment_textures.size(),m_depth_texture);
        if(m_fbo_idx>=0)
        {
            if(getAPIState().target==m_fbo_idx)
            {
                getAPIInterface().resolveTarget(m_fbo_idx);
                getAPIState().target=idx;
            }
            getAPIInterface().removeTarget(m_fbo_idx);
        }
        m_fbo_idx=idx;
        m_update=false;
    }

    getAPIState().target=m_fbo_idx;
}

void RoxFBO::unbind()
{
    if(getAPIState().target>=0)
        getAPIInterface().resolveTarget(getAPIState().target);

    getAPIState().target=-1;
}

const RoxFBO RoxFBO::getCurrent() { RoxFBO f; f.m_fbo_idx=getAPIState().target; return f; }

unsigned int RoxFBO::getMaxColorAttachments() { return getAPIInterface().getMaxTargetAttachments(); }
unsigned int RoxFBO::getMaxMSAA() { return getAPIInterface().getMaxTargetMsaa(); }

}
