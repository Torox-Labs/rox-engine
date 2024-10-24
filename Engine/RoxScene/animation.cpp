//nya-engine (C) nyan.developer@gmail.com released under the MIT license (see LICENSE)

#include "animation.h"
#include "RoxMemory/RoxMemoryReader.h"
#include "RoxFormats/RoxAnim.h"

namespace RoxScene
{

bool animation::load(const char *name)
{
    default_load_function(load_nan);

    if(!scene_shared<shared_animation>::load(name))
        return false;

    if(!m_shared.isValid())
        return false;

    m_range_from=0;
    m_range_to=m_shared->anim.getDuration();
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
    m_range_to=m_shared->anim.getDuration();
    m_speed=m_weight=1.0f;
    update_version();
    m_mask.free();
}

bool animation::load_nan(shared_animation &res,resource_data &data,const char* name)
{
    RoxFormats::RoxAnim ran;
    if(!ran.read(data.getData(),data.getSize()))
        return false;

    res.anim.release();
    for(size_t i=0;i<ran.posVec3LinearCurves.size();++i)
    {
        const int bone_idx=res.anim.addBone(ran.posVec3LinearCurves[i].boneName.c_str());
        for(size_t j=0;j<ran.posVec3LinearCurves[i].frames.size();++j)
        {
            const RoxFormats::RoxAnim::PosVec3LinearFrame &f=ran.posVec3LinearCurves[i].frames[j];
            res.anim.addBonePosFrame(bone_idx,f.time,f.pos);
        }
    }

    for(size_t i=0;i<ran.rotQuatLinearCurves.size();++i)
    {
        const int bone_idx=res.anim.addBone(ran.rotQuatLinearCurves[i].boneName.c_str());
        for(size_t j=0;j<ran.rotQuatLinearCurves[i].frames.size();++j)
        {
            const RoxFormats::RoxAnim::RotQuatLinearFrame &f=ran.rotQuatLinearCurves[i].frames[j];
            res.anim.addBoneRotFrame(bone_idx,f.time,f.rot);
        }
    }

    for(size_t i=0;i<ran.floatLinearCurves.size();++i)
    {
        const int bone_idx=res.anim.addCurve(ran.floatLinearCurves[i].boneName.c_str());
        for(size_t j=0;j<ran.floatLinearCurves[i].frames.size();++j)
        {
            const RoxFormats::RoxAnim::FloatLinearFrame &f=ran.floatLinearCurves[i].frames[j];
            res.anim.addCurveFrame(bone_idx,f.time,f.value);
        }
    }

    return true;
}

unsigned int animation::get_duration() const
{
    if(!m_shared.isValid())
        return 0;

    return m_shared->anim.getDuration();
}

void animation::set_range(unsigned int from,unsigned int to)
{
    if(!m_shared.isValid())
        return;

    m_range_from=from;
    m_range_to=to;

    unsigned int duration = m_shared->anim.getDuration();
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
        if(m_mask.isValid())
        {
            m_mask.free();
            update_version();
        }
    }
    else
    {
        if(!m_mask.isValid())
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

    if(!m_shared.isValid())
        return;

    if(m_shared->anim.getBoneIdx(name)<0)
        return;

    if(enabled)
    {
        if(!m_mask.isValid())
            return;

        m_mask->data[name]=true;
        update_version();
    }
    else
    {
        if(!m_mask.isValid())
        {
            m_mask.allocate();
            for(int i=0;i<m_shared->anim.getBonesCount();++i)
                m_mask->data[m_shared->anim.getBoneName(i)]=true;
        }

        std::map<std::string,bool>::iterator it=m_mask->data.find(name);
        if(it!=m_mask->data.end())
            m_mask->data.erase(it);

        update_version();
    }
}

}
