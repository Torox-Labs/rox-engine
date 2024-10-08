//nya-engine (C) nyan.developer@gmail.com released under the MIT license (see LICENSE)

#include "animation.h"
#include "tmb_model.h"
#include "tsb_anim.h"

void applied_animation::apply_anim(const tmb_model &model,const tsb_anim &anim)
{
    m_bones_count=model.get_bones_count();
    m_frames_count=anim.get_frames_count();
    if(!m_frames_count)
    {
        nya_log::log()<<"Unable to set empty animation\n";
        clear();
        return;
    }

    m_first_loop_frame=anim.get_first_loop_frame();

    m_anim_bones.resize(m_frames_count*m_bones_count);

    unsigned int bones_count=m_bones_count;
    if(bones_count>anim.get_bones_count())
        bones_count=anim.get_bones_count();

    if(bones_count<m_bones_count)
        nya_log::log()<<"bones_count<m_bones_count";

    for(unsigned int i=0;i<m_frames_count;++i)
    {
        const nya_math::mat4 *anim_bones=anim.get_bones(i);
        nya_math::mat4 *final_bones=&m_anim_bones[i*m_bones_count];
        
        for(int k=0;k<bones_count;++k)
            final_bones[k]=model.get_bone(k)*anim_bones[k];
        
        for(int k=bones_count;k<m_bones_count;++k)
            final_bones[k]=model.get_bone(k);
    }
}

