//nya-engine (C) nyan.developer@gmail.com released under the MIT license (see LICENSE)

#pragma once

#include "math/vector.h"
#include "math/quaternion.h"

#include <string>
#include <map>
#include <vector>

namespace nya_render
{

class skeleton
{
public:
    int get_bone_idx(const char *name) const; //invalid if < 0
    const char *get_bone_name(int idx) const;
    int get_bone_parent_idx(int idx) const;
    nya_math::vec3 transform(int bone_idx,const nya_math::vec3 &point) const;
    int get_bones_count() const { return (int)m_bones.size(); }

    nya_math::vec3 get_bone_pos(int idx) const;
    nya_math::quat get_bone_rot(int idx) const;
    nya_math::vec3 get_bone_local_pos(int idx) const;
    nya_math::quat get_bone_local_rot(int idx) const;
    nya_math::vec3 get_bone_original_pos(int idx) const;
    nya_math::quat get_bone_original_rot(int idx) const;

    bool has_original_rot() const { return !m_rot_org.empty(); }

    void set_bone_transform(int bone_idx,const nya_math::vec3 &pos,
                                                const nya_math::quat &rot);
    void update();

public:
    const float *get_pos_buffer() const;
    const float *get_rot_buffer() const;

public:
    int add_bone(const char *name,const nya_math::vec3 &pos,
                 const nya_math::quat &rot=nya_math::quat(),int parent_bone_idx= -1,bool allow_doublicate=false);

public:
    int add_ik(int target_bone_idx,int effect_bone_idx,int count,float fact,bool allow_invalid=false);
    bool add_ik_link(int ik_idx,int bone_idx,bool allow_invalid=false);
    bool add_ik_link(int ik_idx,int bone_idx,nya_math::vec3 limit_from,nya_math::vec3 limit_to,bool allow_invalid=false);

public:
    bool add_bound(int bone_idx,int src_bone_idx,float k,bool bound_pos,bool bound_rot,bool allow_invalid=false);

private:
    void update_bone(int idx);
    void base_update_bone(int idx);
    void update_ik(int idx);

private:
    typedef std::map<std::string,unsigned int> index_map;

    struct bone
    {
        nya_math::vec3 pos_org;
        nya_math::vec3 offset;

        nya_math::vec3	pos;
        nya_math::quat	rot;

        int parent;

        short ik_idx;
        short bound_idx;

        std::string name;
    };

    index_map m_bones_map;
    std::vector<bone> m_bones;

    struct bone_r
    {
        nya_math::quat rot_org;
        nya_math::quat offset;
    };

    std::vector<bone_r> m_rot_org;

    std::vector<nya_math::vec3> m_pos_tr;
    std::vector<nya_math::quat> m_rot_tr;

    enum limit_mode
    {
        limit_no,
        limit_x,
        limit_xyz
    };

    struct ik_link
    {
        int idx;
        limit_mode limit;
        nya_math::vec3 limit_from;
        nya_math::vec3 limit_to;
    };

    struct ik
    {
        int target;
        int eff;

        int count;
        float fact;

        std::vector<ik_link> links;
    };

    std::vector<ik> m_iks;

    struct bound
    {
        int src;

        float k;

        bool pos;
        bool rot;
    };

    std::vector<bound> m_bounds;
};

}
