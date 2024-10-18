//nya-engine (C) nyan.developer@gmail.com released under the MIT license (see LICENSE)

#include "animation.h"
#include "memory/invalid_object.h"

namespace
{

template<typename t_list,typename t_frame> void add_frame(nya_memory::shared_ptr<t_list> &seq,const t_frame &f,
                                                          unsigned int time,unsigned int &duration)
{
    if(time>duration)
        duration=time;

    (*seq.operator->())[time]=f;
}

template<typename t_map> int get_idx(const char *name,t_map &map)
{
    if(!name)
        return -1;

    typename t_map::const_iterator it=map.find(name);
    if(it==map.end())
        return -1;

    return it->second;
}

template<typename t_value,typename t_data,typename t_frame> t_value get_value(
                  unsigned int time,bool looped,const nya_memory::shared_ptr<t_data> &seq_sh,unsigned int duration)
{
    const t_data &seq = *seq_sh.operator->();

    if(time>duration)
    {
        if(looped)
        {
            if(duration)
                time=time%duration;
            else
                time=0;
        }
        else
            time=duration;
    }

    typename t_data::const_iterator it_next=seq.lower_bound(time);
    if(it_next==seq.end())
        return seq.empty()?t_value():seq.rbegin()->second.value;

    if(it_next==seq.begin())
        return it_next->second.value;

    typename t_data::const_iterator it=it_next;
    --it;

    const int time_diff=it_next->first-it->first;

    if(time_diff==0)
        return it_next->second.value;

    return it_next->second.interpolate(it->second,float(time-it->first)/time_diff);
}


}

namespace nya_render
{

int animation::get_bone_idx(const char *name) const { return get_idx(name,m_bones_map); }

nya_math::vec3 animation::get_bone_pos(int idx,unsigned int time,bool looped) const
{
    if(idx<0 || idx>=(int)m_bones.size())
        return nya_math::vec3();

    return get_value<nya_math::vec3,pos_sequence,pos_frame>(time,looped,m_bones[idx].pos,m_duration);
}

nya_math::quat animation::get_bone_rot(int idx,unsigned int time,bool looped) const
{
    if(idx<0 || idx>=(int)m_bones.size())
        return nya_math::quat();

    return get_value<nya_math::quat,rot_sequence,rot_frame>(time,looped,m_bones[idx].rot,m_duration);
}

nya_math::vec3 animation::pos_frame::interpolate(const pos_frame &prev,float k) const
{
    return prev.value+nya_math::vec3(inter.x.get(k)*(value.x-prev.value.x),
                                     inter.y.get(k)*(value.y-prev.value.y),
                                     inter.z.get(k)*(value.z-prev.value.z));
}

nya_math::quat animation::rot_frame::interpolate(const rot_frame &prev,float k) const
{
    return nya_math::quat::slerp(prev.value,value,inter.get(k));
}

float animation::curve_frame::interpolate(const curve_frame &prev,float k) const { return prev.value+k*(value-prev.value); }

const char *animation::get_bone_name(int idx) const
{
    if(idx<0 || idx>=(int)m_bones.size())
        return 0;

    return m_bones[idx].name.c_str();
}

int animation::get_curve_idx(const char *name) const { return get_idx(name,m_curves_map); }

float animation::get_curve(int idx,unsigned int time,bool looped) const
{
    if(idx<0 || idx>=(int)m_curves.size())
        return 0.0f;

    return get_value<float,curve_sequence,curve_frame>(time,looped,m_curves[idx].value,m_duration);
}

const char *animation::get_curve_name(int idx) const
{
    if(idx<0 || idx>=(int)m_curves.size())
        return 0;

    return m_curves[idx].name.c_str();
}

int animation::add_bone(const char *name)
{
    if(!name || !name[0])
        return -1;

    const int idx=(int)m_bones.size();
    std::pair<typename index_map::iterator,bool> ret=m_bones_map.insert(std::pair<std::string,int>(name,idx));
    if(ret.second==false)
        return ret.first->second;

    m_bones.push_back(bone(name));
    return idx;
}

void animation::add_bone_pos_frame(int bone_idx,unsigned int time,const nya_math::vec3 &pos,const pos_interpolation &interpolation)
{
    if(bone_idx<0 || bone_idx>=(int)m_bones.size())
        return;

    pos_frame pf;
    pf.value=pos;
    pf.inter=interpolation;
    add_frame(m_bones[bone_idx].pos,pf,time,m_duration);
}

void animation::add_bone_rot_frame(int bone_idx,unsigned int time,const nya_math::quat &rot,const nya_math::bezier &interpolation)
{
    if(bone_idx<0 || bone_idx>=(int)m_bones.size())
        return;

    rot_frame rf;
    rf.value=rot;
    rf.inter=interpolation;
    add_frame(m_bones[bone_idx].rot,rf,time,m_duration);
}

int animation::add_curve(const char *name)
{
    if(!name || !name[0])
        return -1;

    const int idx=(int)m_curves.size();
    std::pair<typename index_map::iterator,bool> ret=m_curves_map.insert(std::pair<std::string,int>(name,idx));
    if(ret.second==false)
        return ret.first->second;
    m_curves.push_back(curve(name));
    return idx;
}

void animation::add_curve_frame(int idx,unsigned int time,float value)
{
    if(idx<0 || idx>=(int)m_curves.size())
        return;

    curve_frame f;
    f.value=value;
    add_frame(m_curves[idx].value,f,time,m_duration);
}

const animation::pos_sequence &animation::get_pos_frames(int idx) const
{
    if(idx<0 || idx>=(int)m_bones.size())
        return nya_memory::invalid_object<pos_sequence>();

    return *m_bones[idx].pos.operator->();
}

const animation::rot_sequence &animation::get_rot_frames(int idx) const
{
    if(idx<0 || idx>=(int)m_bones.size())
        return nya_memory::invalid_object<rot_sequence>();

    return *m_bones[idx].rot.operator->();
}

const animation::curve_sequence &animation::get_curve_frames(int idx) const
{
    if(idx<0 || idx>=(int)m_curves.size())
        return nya_memory::invalid_object<curve_sequence>();

    return *m_curves[idx].value.operator->();
}

}
