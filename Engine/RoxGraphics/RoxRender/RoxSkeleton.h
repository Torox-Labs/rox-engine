// Updated By the ROX_ENGINE
// Copyright (C) 2024 Torox Project
// Portions Copyright (C) 2013 nyan.developer@gmail.com (nya-engine)
//
// This file was modified by the Torox Project.
// Update the render api intefrace to check Metal 1th.
//
// This file incorporates code from the nya-engine project, which is licensed under the MIT License.
// See the LICENSE-MIT file in the root directory for more information.
//
// This file is also part of the Rox-engine, which is licensed under a dual-license system:
// 1. Free Use License (for non-commercial and commercial use under specific conditions)
// 2. Commercial License (for use on proprietary platforms)
// See the LICENSE file in the root directory for the full Rox-engine license terms.

#pragma once

#include "RoxMath/RoxVector.h"
#include "RoxMath/RoxQuaternion.h"

#include <string>
#include <map>
#include <vector>

namespace RoxRender
{

class RoxSkeleton
{
public:
    int getBoneIdx(const char *name) const; //invalid if < 0
    const char *getBoneName(int idx) const;
    int getBoneParentIdx(int idx) const;
    RoxMath::Vector3 transform(int bone_idx,const RoxMath::Vector3 &point) const;
    int getBonesCount() const { return (int)m_bones.size(); }

    RoxMath::Vector3 getBonePos(int idx) const;
    RoxMath::Quaternion getBoneRot(int idx) const;
    RoxMath::Vector3 getBoneLocalPos(int idx) const;
    RoxMath::Quaternion getBoneLocalRot(int idx) const;
    RoxMath::Vector3 getBoneOriginalPos(int idx) const;
    RoxMath::Quaternion getBoneOriginalRot(int idx) const;

    bool hasOriginalRot() const { return !m_rot_org.empty(); }

    void setBoneTransform(int bone_idx,const RoxMath::Vector3 &pos,
                                                const RoxMath::Quaternion &rot);
    void update();

public:
    const float *getPosBuffer() const;
    const float *getRotBuffer() const;

public:
    int addBone(const char *name,const RoxMath::Vector3 &pos,
                 const RoxMath::Quaternion &rot=RoxMath::Quaternion(),int parent_bone_idx= -1,bool allow_doublicate=false);

public:
    int addIk(int target_bone_idx,int effect_bone_idx,int count,float fact,bool allow_invalid=false);
    bool addIkLink(int ik_idx,int bone_idx,bool allow_invalid=false);
    bool addIkLink(int ik_idx,int bone_idx,RoxMath::Vector3 limit_from,RoxMath::Vector3 limit_to,bool allow_invalid=false);

public:
    bool addBound(int bone_idx,int src_bone_idx,float k,bool bound_pos,bool bound_rot,bool allow_invalid=false);

private:
    void updateBone(int idx);
    void baseUpdateBone(int idx);
    void updateIk(int idx);

private:
    typedef std::map<std::string,unsigned int> index_map;

    struct bone
    {
        RoxMath::Vector3 pos_org;
        RoxMath::Vector3 offset;

        RoxMath::Vector3	pos;
        RoxMath::Quaternion	rot;

        int parent;

        short ik_idx;
        short bound_idx;

        std::string name;
    };

    index_map m_bones_map;
    std::vector<bone> m_bones;

    struct bone_r
    {
        RoxMath::Quaternion rot_org;
        RoxMath::Quaternion offset;
    };

    std::vector<bone_r> m_rot_org;

    std::vector<RoxMath::Vector3> m_pos_tr;
    std::vector<RoxMath::Quaternion> m_rot_tr;

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
        RoxMath::Vector3 limit_from;
        RoxMath::Vector3 limit_to;
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
