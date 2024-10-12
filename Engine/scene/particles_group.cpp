//nya-engine (C) nyan.developer@gmail.com released under the MIT license (see LICENSE)

#include "particles_group.h"
#include "formats/text_parser.h"
#include "formats/string_convert.h"
#include "memory/invalid_object.h"
#include "stdlib.h"

namespace nya_scene
{

bool particles_group::load(const char *name)
{
    default_load_function(load_text);
    if(!scene_shared::load(name))
        return false;

    m_particles=get_shared_data()->particles;
    m_transform=transform();
    return true;
}

void particles_group::unload()
{
    m_particles.clear();
    m_transform=transform();
}

void particles_group::reset_time()
{
    for(int i=0;i<get_count();++i)
        m_particles[i].reset_time();
}

void particles_group::update(unsigned int dt)
{
    for(int i=0;i<get_count();++i)
        m_particles[i].update(dt);
}

void particles_group::draw(const char *pass_name) const
{
    for(int i=0;i<get_count();++i)
        m_particles[i].draw(pass_name);
}

void particles_group::set_pos(const nya_math::vec3 &pos)
{
    m_transform.set_pos(pos);
    for(int i=0;i<get_count();++i)
        m_particles[i].set_pos(pos);
}

void particles_group::set_rot(const nya_math::quat &rot)
{
    m_transform.set_rot(rot);
    for(int i=0;i<get_count();++i)
        m_particles[i].set_rot(rot);
}

void particles_group::set_rot(nya_math::angle_deg yaw,nya_math::angle_deg pitch,nya_math::angle_deg roll)
{
    m_transform.set_rot(yaw,pitch,roll);
    for(int i=0;i<get_count();++i)
        m_particles[i].set_rot(m_transform.get_rot());
}

void particles_group::set_scale(float s)
{
    m_transform.set_scale(s,s,s);
    for(int i=0;i<get_count();++i)
        m_particles[i].set_scale(s);
}

void particles_group::set_scale(const nya_math::vec3 &s)
{
    m_transform.set_scale(s.x,s.y,s.z);
    for(int i=0;i<get_count();++i)
        m_particles[i].set_scale(s);
}

int particles_group::get_count() const
{
    return (int)m_particles.size();
}

const particles &particles_group::get(int idx) const
{
    if(idx<0 || idx>=get_count())
        return nya_memory::invalid_object<particles>();

    return m_particles[idx];
}

bool particles_group::load_text(shared_particles_group &res,resource_data &data,const char* name)
{
    nya_formats::text_parser parser;
    if(!parser.load_from_data((char *)data.get_data(),data.get_size()))
        return false;

    for(int i=0;i<parser.get_sections_count();++i)
    {
        if(parser.is_section_type(i,"particles"))
        {
            res.particles.push_back(particles(parser.get_section_name(i)));
            for(int j=0;j<parser.get_subsections_count(i);++j)
            {
                const char *type=parser.get_subsection_type(i,j);
                if(!type || !type[0])
                    continue;

                particles &p=res.particles.back();
                p.set_param(type,parser.get_subsection_value_vec4(i,j));
            }
        }
        else if(parser.is_section_type(i,"param"))
        {
            const char *pname=parser.get_section_name(i);
            if(!pname || !pname[0])
                continue;

            res.params.push_back(std::make_pair(pname,parser.get_section_value_vec4(i)));
        }
    }

    for(int i=0;i<(int)res.particles.size();++i)
    {
        for(int j=0;j<(int)res.params.size();++j)
        {
            particles &p=res.particles[i];
            p.set_param(res.params[j].first.c_str(),res.params[j].second);
        }
    }

    return true;
}

}
