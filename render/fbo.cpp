//nya-engine (C) nyan.developer@gmail.com released under the MIT license (see LICENSE)

#include "fbo.h"
#include "render_api.h"

namespace nya_render
{

void fbo::set_color_target(const texture &tex,cubemap_side side,unsigned int attachment_idx,unsigned int samples)
{
    if(attachment_idx>=get_max_color_attachments())
        return;

    if(samples>get_max_msaa())
        samples=get_max_msaa();

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

void fbo::set_color_target(const texture &tex,unsigned int attachment_idx,unsigned int samples)
{
    set_color_target(tex,cubemap_side(-1),attachment_idx,samples);
}

void fbo::set_depth_target(const texture &tex)
{
    m_depth_texture=tex.m_tex;
    if(tex.m_tex>=0)
        m_width=tex.m_width,m_height=tex.m_height;
    m_update=true;
}

void fbo::release()
{
	if(m_fbo_idx<0)
		return;

    get_api_interface().remove_target(m_fbo_idx);
    if(get_api_state().target==m_fbo_idx)
        get_api_state().target=-1;

    *this=fbo();
}

void fbo::bind() const
{
    if(get_api_state().target>=0 && get_api_state().target!=m_fbo_idx)
        get_api_interface().resolve_target(get_api_state().target);

    if(m_update)
    {
        const int idx=get_api_interface().create_target(m_width,m_height,m_samples,m_attachment_textures.data(),
                                                        m_attachment_sides.data(),(int)m_attachment_textures.size(),m_depth_texture);
        if(m_fbo_idx>=0)
        {
            if(get_api_state().target==m_fbo_idx)
            {
                get_api_interface().resolve_target(m_fbo_idx);
                get_api_state().target=idx;
            }
            get_api_interface().remove_target(m_fbo_idx);
        }
        m_fbo_idx=idx;
        m_update=false;
    }

    get_api_state().target=m_fbo_idx;
}

void fbo::unbind()
{
    if(get_api_state().target>=0)
        get_api_interface().resolve_target(get_api_state().target);

    get_api_state().target=-1;
}

const fbo fbo::get_current() { fbo f; f.m_fbo_idx=get_api_state().target; return f; }

unsigned int fbo::get_max_color_attachments() { return get_api_interface().get_max_target_attachments(); }
unsigned int fbo::get_max_msaa() { return get_api_interface().get_max_target_msaa(); }

}
