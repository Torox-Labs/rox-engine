//nya-engine (C) nyan.developer@gmail.com released under the MIT license (see LICENSE)

#include "animation.h"
#include "memory/memory_reader.h"
#include "formats/nan.h"

namespace nya_scene
{

bool animation::load(const char *name)
{
    default_load_function(load_nan);

    if(!scene_shared<shared_animation>::load(name))
        return false;

    if(!m_shared.is_valid())
        return false;

    m_range_from=0;
    m_range_to=m_shared->anim.get_duration();
    m_speed=m_weight=1.0f;
    update_version();
    m_mask.free();

    return true;
}

void animation::unload()
{
    scene_shared<shared_animation>::unload();

    m_range_from=m_range_to=0;
    m_speed=m_weight=1.0f;

    m_mask.free();
}

void animation::create(const shared_animation &res)
{
    scene_shared::create(res);

    m_range_from=0;
    m_range_to=m_shared->anim.get_duration();
    m_speed=m_weight=1.0f;
    update_version();
    m_mask.free();
}

bool animation::load_nan(shared_animation &res,resource_data &data,const char* name)
{
    nya_formats::nan nan;
    if(!nan.read(data.get_data(),data.get_size()))
        return false;

    res.anim.release();
    for(size_t i=0;i<nan.pos_vec3_linear_curves.size();++i)
    {
        const int bone_idx=res.anim.add_bone(nan.pos_vec3_linear_curves[i].bone_name.c_str());
        for(size_t j=0;j<nan.pos_vec3_linear_curves[i].frames.size();++j)
        {
            const nya_formats::nan::pos_vec3_linear_frame &f=nan.pos_vec3_linear_curves[i].frames[j];
            res.anim.add_bone_pos_frame(bone_idx,f.time,f.pos);
        }
    }

    for(size_t i=0;i<nan.rot_quat_linear_curves.size();++i)
    {
        const int bone_idx=res.anim.add_bone(nan.rot_quat_linear_curves[i].bone_name.c_str());
        for(size_t j=0;j<nan.rot_quat_linear_curves[i].frames.size();++j)
        {
            const nya_formats::nan::rot_quat_linear_frame &f=nan.rot_quat_linear_curves[i].frames[j];
            res.anim.add_bone_rot_frame(bone_idx,f.time,f.rot);
        }
    }

    for(size_t i=0;i<nan.float_linear_curves.size();++i)
    {
        const int bone_idx=res.anim.add_curve(nan.float_linear_curves[i].bone_name.c_str());
        for(size_t j=0;j<nan.float_linear_curves[i].frames.size();++j)
        {
            const nya_formats::nan::float_linear_frame &f=nan.float_linear_curves[i].frames[j];
            res.anim.add_curve_frame(bone_idx,f.time,f.value);
        }
    }

    return true;
}

unsigned int animation::get_duration() const
{
    if(!m_shared.is_valid())
        return 0;

    return m_shared->anim.get_duration();
}

void animation::set_range(unsigned int from,unsigned int to)
{
    if(!m_shared.is_valid())
        return;

    m_range_from=from;
    m_range_to=to;

    unsigned int duration = m_shared->anim.get_duration();
    if(m_range_from>duration)
        m_range_from=duration;

    if(m_range_to>duration)
        m_range_to=duration;

    if(m_range_from>m_range_to)
        m_range_from=m_range_to;
}

void animation::mask_all(bool enabled)
{
    if(enabled)
    {
        if(m_mask.is_valid())
        {
            m_mask.free();
            update_version();
        }
    }
    else
    {
        if(!m_mask.is_valid())
            m_mask.allocate();
        else
            m_mask->data.clear();

        update_version();
    }
}

void animation::update_version()
{
    static unsigned int version = 0;
    m_version= ++version;
}

void animation::add_mask(const char *name,bool enabled)
{
    if(!name)
        return;

    if(!m_shared.is_valid())
        return;

    if(m_shared->anim.get_bone_idx(name)<0)
        return;

    if(enabled)
    {
        if(!m_mask.is_valid())
            return;

        m_mask->data[name]=true;
        update_version();
    }
    else
    {
        if(!m_mask.is_valid())
        {
            m_mask.allocate();
            for(int i=0;i<m_shared->anim.get_bones_count();++i)
                m_mask->data[m_shared->anim.get_bone_name(i)]=true;
        }

        std::map<std::string,bool>::iterator it=m_mask->data.find(name);
        if(it!=m_mask->data.end())
            m_mask->data.erase(it);

        update_version();
    }
}

}
