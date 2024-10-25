//nya-engine (C) nyan.developer@gmail.com released under the MIT license (see LICENSE)

#include "particles_group.h"
#include "RoxFormats/RoxTextParser.h"
#include "RoxFormats/RoxStringConvert.h"
#include "RoxMemory/RoxInvalidObject.h"
#include "stdlib.h"

namespace RoxScene
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

void particles_group::set_pos(const RoxMath::Vector3 &pos)
{
    m_transform.set_pos(pos);
    for(int i=0;i<get_count();++i)
        m_particles[i].set_pos(pos);
}

void particles_group::set_rot(const RoxMath::Quaternion &rot)
{
    m_transform.set_rot(rot);
    for(int i=0;i<get_count();++i)
        m_particles[i].set_rot(rot);
}

void particles_group::set_rot(RoxMath::AngleDeg yaw,RoxMath::AngleDeg pitch,RoxMath::AngleDeg roll)
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

void particles_group::set_scale(const RoxMath::Vector3 &s)
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
        return RoxMemory::invalidObject<particles>();

    return m_particles[idx];
}

bool particles_group::load_text(shared_particles_group &res,resource_data &data,const char* name)
{
    RoxFormats::RTextParser parser;
    if(!parser.loadFromData((char *)data.getData(),data.getSize()))
        return false;

    for(int i=0;i<parser.getSectionsCount();++i)
    {
        if(parser.isSectionType(i,"particles"))
        {
            res.particles.push_back(particles(parser.getSectionName(i)));
            for(int j=0;j<parser.getSubsectionsCount(i);++j)
            {
                const char *type=parser.getSubsectionType(i,j);
                if(!type || !type[0])
                    continue;

                particles &p=res.particles.back();
                p.set_param(type,parser.getSubsectionValueVec4(i,j));
            }
        }
        else if(parser.isSectionType(i,"param"))
        {
            const char *pname=parser.getSectionName(i);
            if(!pname || !pname[0])
                continue;

            res.params.push_back(std::make_pair(pname,parser.getSectionValueVec4(i)));
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
