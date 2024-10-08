//nya-engine (C) nyan.developer@gmail.com released under the MIT license (see LICENSE)

#pragma once

#include "math/matrix.h"
#include <vector>

class tsb_anim;
class tmb_model;

class applied_animation
{
public:
    const float *get_buffer(unsigned int frame) const
    {
        if(m_anim_bones.empty() || !m_frames_count)
            return 0;
        
        return m_anim_bones[m_bones_count*(frame % m_frames_count)].m[0];
    }

    unsigned int get_frames_count() const { return m_frames_count; };
    unsigned int get_bones_count() const { return m_bones_count; }
    unsigned int get_first_loop_frame() const { return m_first_loop_frame; }

    void apply_anim(const tmb_model &model,const tsb_anim &anim);

    void clear()
    {
        m_bones_count=0;
        m_frames_count=0;
        m_first_loop_frame=0;
        m_anim_bones.clear();
    }

    applied_animation(): m_frames_count(0),m_bones_count(0) {}

private:
    unsigned int m_frames_count;
    unsigned int m_bones_count;
    unsigned int m_first_loop_frame;
    std::vector<nya_math::mat4> m_anim_bones;
};
