//nya-engine (C) nyan.developer@gmail.com released under the MIT license (see LICENSE)

#include "skeleton.h"

namespace nya_render
{

int skeleton::add_bone(const char *name,const nya_math::vec3 &pos,const nya_math::quat &rot,int parent,bool allow_doublicate)
{
    if(!name)
        return -1;

    if(parent>=(int)m_bones.size())
        return -1;

    const int bone_idx=(int)m_bones.size();
    std::pair<index_map::iterator,bool> ret=
            m_bones_map.insert (std::pair<std::string,int>(name,bone_idx));

    if(!allow_doublicate && ret.second==false)
        return ret.first->second;

    m_bones.resize(bone_idx+1);
    m_pos_tr.resize(bone_idx+1);
    m_rot_tr.resize(bone_idx+1);

    if(!m_rot_org.empty() || rot.v.length_sq()>0.001f)
        m_rot_org.resize(m_bones.size());

    bone &b=m_bones[bone_idx];
    b.name.assign(name);
    b.parent=parent;
    b.ik_idx=b.bound_idx= -1;
    b.pos_org=pos;

    if(parent>=0)
    {
        const bone &p=m_bones[parent];
        b.offset=pos-p.pos_org;
    }
    else
        b.offset=pos;

    if(!m_rot_org.empty())
    {
        m_rot_org[bone_idx].rot_org=rot;
        if(parent>=0)
        {
            const nya_math::quat pq=nya_math::quat::invert(m_rot_org[parent].rot_org);
            m_rot_org[bone_idx].offset=pq*m_rot_org[bone_idx].rot_org;
            b.offset=pq.rotate(b.offset);
        }
        else
            m_rot_org[bone_idx].offset=m_rot_org[bone_idx].rot_org;
    }

    update_bone(bone_idx);
    return bone_idx;
}

void skeleton::base_update_bone(int idx)
{
    const bone &b=m_bones[idx];
    if(b.parent>=0)
    {
        m_pos_tr[idx]=m_pos_tr[b.parent] + m_rot_tr[b.parent].rotate(b.pos+b.offset);

        if(m_rot_org.empty())
            m_rot_tr[idx]=m_rot_tr[b.parent]*b.rot;
        else
            m_rot_tr[idx]=m_rot_tr[b.parent]*(m_rot_org[idx].offset*b.rot);
    }
    else
    {
        m_pos_tr[idx]=b.pos+b.offset;
        if(m_rot_org.empty())
            m_rot_tr[idx]=b.rot;
        else
            m_rot_tr[idx]=m_rot_org[idx].offset*b.rot;
    }
}

void skeleton::update_bone(int idx)
{
    bone &b=m_bones[idx];
    if(b.bound_idx>=0)
    {
        const bound &bnd=m_bounds[b.bound_idx];

        const nya_math::vec3 prev_pos=b.pos;
        const nya_math::quat prev_rot=b.rot;

        const bone &src=m_bones[bnd.src];

        if(bnd.pos)
            b.pos+=src.pos*bnd.k;

        if(bnd.rot)
        {
            nya_math::quat tmp=src.rot;
            tmp.apply_weight(bnd.k);
            b.rot=(b.rot*tmp).normalize();
        }
        base_update_bone(idx);
        b.pos=prev_pos;
        b.rot=prev_rot;
    }
    else
        base_update_bone(idx);

    if(b.ik_idx>=0)
        update_ik(b.ik_idx);
}

int skeleton::get_bone_idx(const char *name) const
{
    if(!name)
        return -1;

    index_map::const_iterator it=m_bones_map.find(name);
    if(it==m_bones_map.end())
        return -1;

    return (int)it->second;
}

int skeleton::get_bone_parent_idx(int idx) const
{
    if(idx<0 || idx>=(int)m_bones.size())
        return -1;

    return m_bones[idx].parent;
}

const char *skeleton::get_bone_name(int idx) const
{
    if(idx<0 || idx>=(int)m_bones.size())
        return 0;

    return m_bones[idx].name.c_str();
}

nya_math::vec3 skeleton::get_bone_pos(int idx) const
{
    if(idx<0 || idx>=(int)m_bones.size())
        return nya_math::vec3();

    return m_pos_tr[idx];
}

nya_math::quat skeleton::get_bone_rot(int idx) const
{
    if(idx<0 || idx>=(int)m_bones.size())
        return nya_math::quat();

    return m_rot_tr[idx];
}

nya_math::vec3 skeleton::get_bone_local_pos(int idx) const
{
    if(idx<0 || idx>=(int)m_bones.size())
        return nya_math::vec3();

    return m_bones[idx].pos;
}

nya_math::quat skeleton::get_bone_local_rot(int idx) const
{
    if(idx<0 || idx>=(int)m_bones.size())
        return nya_math::quat();

    return m_bones[idx].rot;
}

nya_math::vec3 skeleton::get_bone_original_pos(int idx) const
{
    if(idx<0 || idx>=(int)m_bones.size())
        return nya_math::vec3();

    return m_bones[idx].pos_org;
}

nya_math::quat skeleton::get_bone_original_rot(int idx) const
{
    if(idx<0 || idx>=(int)m_rot_org.size())
        return nya_math::quat();

    return m_rot_org[idx].rot_org;
}

int skeleton::add_ik(int target_bone_idx,int effect_bone_idx,int count,float fact,bool allow_invalid)
{
    if(target_bone_idx<0 || (!allow_invalid && target_bone_idx>=(int)m_bones.size()))
        return -1;

    if(effect_bone_idx<0 || (!allow_invalid && effect_bone_idx>=(int)m_bones.size()))
        return -1;

    const int ik_idx=(int)m_iks.size();
    m_iks.resize(ik_idx+1);

    ik &k=m_iks[ik_idx];
    k.target=target_bone_idx;
    k.eff=effect_bone_idx;
    k.count=count;
    k.fact=fact;

    m_bones[target_bone_idx].ik_idx=(short)ik_idx;

    return ik_idx;
}

bool skeleton::add_ik_link(int ik_idx,int bone_idx,bool allow_invalid)
{
    if(ik_idx<0 || ik_idx>=(int)m_iks.size())
        return false;

    if(bone_idx<0 || (!allow_invalid && bone_idx>=(int)m_bones.size()))
        return false;

    ik &k=m_iks[ik_idx];
    k.links.resize(k.links.size()+1);
    k.links.back().idx=bone_idx;
    k.links.back().limit=limit_no;

    return true;
}

bool skeleton::add_ik_link(int ik_idx,int bone_idx,nya_math::vec3 limit_from,nya_math::vec3 limit_to,bool allow_invalid)
{
    if(ik_idx<0 || ik_idx>=(int)m_iks.size())
        return false;

    if(bone_idx<0 || (!allow_invalid && bone_idx>=(int)m_bones.size()))
        return false;

    ik &k=m_iks[ik_idx];
    k.links.resize(k.links.size()+1);
    k.links.back().idx=bone_idx;
    k.links.back().limit_from=limit_from;
    k.links.back().limit_to=limit_to;

    const nya_math::vec3 limit=limit_from.abs()+limit_to.abs();
    const float eps=0.01f;
    if(limit.x>eps && limit.y<eps && limit.z<eps)
        k.links.back().limit=limit_x;
    else if(limit.x<eps && limit.y<eps && limit.z<eps)
        k.links.back().limit=limit_no;
    else
        k.links.back().limit=limit_xyz;

    return true;
}

bool skeleton::add_bound(int bone_idx,int src_bone_idx,float k,bool pos,bool rot,bool allow_invalid)
{
    if(bone_idx<0 || (!allow_invalid && bone_idx>=(int)m_bones.size()))
        return false;

    if(src_bone_idx<0 || (!allow_invalid && src_bone_idx>=(int)m_bones.size()))
        return false;

    if((!pos && !rot) || fabsf(k)<0.001f)
        return false;

    m_bones[bone_idx].bound_idx=(short)m_bounds.size();
    m_bounds.resize(m_bounds.size()+1);
    bound &b=m_bounds.back();
    b.src=src_bone_idx;
    b.k=k;
    b.pos=pos;
    b.rot=rot;

    return true;
}

void skeleton::set_bone_transform(int bone_idx,const nya_math::vec3 &pos,const nya_math::quat &rot)
{
    if(bone_idx<0 || bone_idx>=(int)m_bones.size())
        return;

    bone &b=m_bones[bone_idx];
    b.pos=pos;
    b.rot=rot;
}

inline void restrict_angle(float &a,float from,float to)
{
    if(a<from)
    {
        const float tmp=2.0f*from-a;
        a=(tmp>to)?from:tmp;
    }
    if(a>to)
    {
        const float tmp=2.0f*to-a;
        a=(tmp<from)?to:tmp;
    }
}

void skeleton::update_ik(int idx)
{
    const ik &k=m_iks[idx];
    const nya_math::vec3 target_pos=m_pos_tr[k.target];

    for(int i=0;i<k.count;++i)
    {
        for(int j=0;j<(int)k.links.size();++j)
        {
            const int lnk_idx=k.links[j].idx;
            bone &lnk=m_bones[lnk_idx];

            const nya_math::vec3 target_dir=(target_pos-m_pos_tr[lnk_idx]).normalize();
            const nya_math::vec3 eff_dir=(m_pos_tr[k.eff]-m_pos_tr[lnk_idx]).normalize();
            if((eff_dir-target_dir).length_sq()<1.0e-7f)
            {
                i=k.count;
                break;
            }

            if(k.links[j].limit==limit_x && k.links.size()==2 && k.links[1].limit==limit_no)
            {
                const ik_link &l0=k.links[0];

                const float target_lsq=(target_pos-m_pos_tr[k.links[1].idx]).length_sq();
                const float l0_lsq=(m_pos_tr[k.eff]-m_pos_tr[l0.idx]).length_sq();
                const float l1_lsq=(m_pos_tr[l0.idx]-m_pos_tr[k.links[1].idx]).length_sq();

                const float l0l1=sqrtf(l0_lsq)+sqrtf(l1_lsq);
                if(l0l1*l0l1>target_lsq)
                {
                    float pitch;
                    const float d=sqrtf(l0_lsq*l1_lsq)*2.0f;
                    const float k=(l0l1-sqrtf(target_lsq))/l0l1;
                    const float small_k=0.1f;
                    if(k<small_k)
                    {
                        const float t=l0l1*(1.0f-small_k);
                        float small_pitch=nya_math::constants::pi-acosf((l0_lsq+l1_lsq-t*t)/d);
                        pitch=small_pitch*k/small_k;
                    }
                    else
                        pitch=nya_math::constants::pi-acosf((l0_lsq+l1_lsq-target_lsq)/d);
                    restrict_angle(pitch,l0.limit_from.x,l0.limit_to.x);
                    lnk.rot=nya_math::quat(pitch,0.0f,0.0f);
                }
                else
                    lnk.rot=nya_math::quat();
            }
            else
            {
                const nya_math::vec3 axis=m_rot_tr[lnk_idx].rotate_inv(nya_math::vec3::cross(eff_dir,target_dir)).normalize();
                const float d=nya_math::clamp(eff_dir.dot(target_dir),-1.0f,1.0f);
                const nya_math::angle_rad ang=nya_math::min(acosf(d),k.fact*(i+1)*2.0f);
                lnk.rot*=nya_math::quat(axis,ang);

                if(k.links[j].limit==limit_xyz)
                {
                    const nya_math::quat &q=lnk.rot;
                    nya_math::vec3 euler(atan2(2.0f*(q.v.y*q.v.z+q.w*q.v.x), 1.0f-2.0f*(q.v.x*q.v.x+q.v.y*q.v.y)),
                                        -asinf(2.0f*(q.v.x*q.v.z-q.w*q.v.y)),
                                         atan2(2.0f*(q.v.x*q.v.y+q.w*q.v.z), 1.0f-2.0f*(q.v.y*q.v.y+q.v.z*q.v.z)));

                    restrict_angle(euler.x,k.links[j].limit_from.x,k.links[j].limit_to.x);
                    restrict_angle(euler.y,k.links[j].limit_from.y,k.links[j].limit_to.y);
                    restrict_angle(euler.z,k.links[j].limit_from.z,k.links[j].limit_to.z);
                    lnk.rot=nya_math::quat(euler.x,euler.y,euler.z);
                }
            }

            for(int m=j;m>=0;--m)
                base_update_bone(k.links[m].idx);

            base_update_bone(k.eff);
        }
    }
}

void skeleton::update()
{
    if(m_iks.empty() && m_bounds.empty())
    {
        for(int i=0,count=(int)m_bones.size();i<count;++i)
            base_update_bone(i);
    }
    else
    {
        for(int i=0,count=(int)m_bones.size();i<count;++i)
            update_bone(i);
    }
}

nya_math::vec3 skeleton::transform(int bone_idx,const nya_math::vec3 &point) const
{
    if(bone_idx<0 || bone_idx>=(int)m_bones.size())
        return point;

    return m_pos_tr[bone_idx]+m_rot_tr[bone_idx].rotate(point);
}

const float *skeleton::get_pos_buffer() const
{
    if(m_pos_tr.empty())
        return 0;

    return &m_pos_tr[0].x;
}

const float *skeleton::get_rot_buffer() const
{
    if(m_rot_tr.empty())
        return 0;

    return &m_rot_tr[0].v.x;
}

}
