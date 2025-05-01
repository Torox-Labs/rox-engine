// Updated By the ROX_ENGINE
// Copyright (C) 2024 Torox Project
// Portions Copyright (C) 2013 nyan.developer@gmail.com (nya-engine)
//
// This file was modified by the Torox Project.
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
#include "RoxMath/RoxBezier.h"
#include "RoxMemory/RoxSharedPtr.h"

#include <vector>
#include <map>
#include <string>

namespace RoxRender
{

class RoxAnimation
{
public:
    unsigned int getDuration() const { return m_duration; }
    int getBoneIdx(const char *name) const; //< 0 if invalid
    RoxMath::Vector3 getBonePos(int idx,unsigned int time,bool looped=true) const;
    RoxMath::Quaternion getBoneRot(int idx,unsigned int time,bool looped=true) const;
    int getBonesCount() const { return (int)m_bones.size(); }
    const char *getBoneName(int idx) const;

    int getCurveIdx(const char *name) const; //< 0 if invalid
    float getCurve(int idx,unsigned int time,bool looped=true) const;
    int get_cuvesCount() const { return (int)m_curves.size(); }
    const char *getCurveName(int idx) const;

public:
    int addBone(const char *name); //create or return existing
    void addBonePosFrame(int bone_idx,unsigned int time,const RoxMath::Vector3 &pos) { pos_interpolation i; addBonePosFrame(bone_idx,time,pos,i); }
    void addBoneRotFrame(int bone_idx,unsigned int time,const RoxMath::Quaternion &rot) { RoxMath::RoxBezier i; addBoneRotFrame(bone_idx,time,rot,i); }

    struct pos_interpolation
    {
        RoxMath::RoxBezier x;
        RoxMath::RoxBezier y;
        RoxMath::RoxBezier z;
    };

    void addBonePosFrame(int bone_idx,unsigned int time,const RoxMath::Vector3 &pos,const pos_interpolation &interpolation);
    void addBoneRotFrame(int bone_idx,unsigned int time,const RoxMath::Quaternion &rot,const RoxMath::RoxBezier &interpolation);

public:
    int addCurve(const char *name); //create or return existing
    void addCurveFrame(int idx,unsigned int time,float value);

public:
    template<typename t,typename interpolation>struct frame { t value; interpolation inter; };
    struct pos_frame: public frame<RoxMath::Vector3,pos_interpolation> { RoxMath::Vector3 interpolate(const pos_frame &prev,float k) const; };
    struct rot_frame: public frame<RoxMath::Quaternion,RoxMath::RoxBezier> { RoxMath::Quaternion interpolate(const rot_frame &prev,float k) const; };
    struct curve_frame { float value; float interpolate(const curve_frame &prev,float k) const; };
    typedef std::map<unsigned int,pos_frame> pos_sequence;
    typedef std::map<unsigned int,rot_frame> rot_sequence;
    typedef std::map<unsigned int,curve_frame> curve_sequence;

    const pos_sequence &getPosFrames(int bone_idx) const;
    const rot_sequence &getRotFrames(int bone_idx) const;
    const curve_sequence &getCurveFrames(int curve_idx) const;

public:
    void release() { *this=RoxAnimation(); }

public:
    RoxAnimation(): m_duration(0) {}

private:
    typedef std::map<std::string,unsigned int> index_map;
    index_map m_bones_map;
    struct bone
    {
        std::string name;
        RoxMemory::RoxSharedPtr<pos_sequence> pos;
        RoxMemory::RoxSharedPtr<rot_sequence> rot;
        bone() {}
        bone(const char *name): name(name) { pos.create(); rot.create(); }
    };
    std::vector<bone> m_bones;

    index_map m_curves_map;
    struct curve
    {
        std::string name;
        RoxMemory::RoxSharedPtr<curve_sequence> value;
        curve() {}
        curve(const char *name): name(name) { value.create(); }
    };
    std::vector<curve> m_curves;

    unsigned int m_duration;
};

}
