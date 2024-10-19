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

#include "RoxFrustum.h"
#include "RoxQuaternion.h"

namespace RoxMath
{

Aabb::Aabb(const Vector3 &Aabb_min,const Vector3 &Aabb_max)
{
    delta=(Aabb_max-Aabb_min)*0.5f;
    origin=Aabb_min+delta;
}

Aabb::Aabb(const Aabb &b,const Vector3 &p,const Quaternion &q,const Vector3 &s)
{
    delta = b.delta * s;
    const Vector3 v[4]=
    {
        Vector3(delta.x,delta.y,delta.z),
        Vector3(delta.x,-delta.y,delta.z),
        Vector3(delta.x,delta.y,-delta.z),
        Vector3(delta.x,-delta.y,-delta.z)
    };
    delta=Vector3();
    for(int i=0;i<4;++i)
        delta=Vector3::max(q.rotate(v[i]).abs(),delta);
    origin=q.rotate(b.origin * s)+p;
}

inline Vector3 scale_rotate(const Matrix4 &m, const RoxMath::Vector3 &v)
{
    return Vector3(m[0][0]*v.x+m[1][0]*v.y+m[2][0]*v.z,
                m[0][1]*v.x+m[1][1]*v.y+m[2][1]*v.z,
                m[0][2]*v.x+m[1][2]*v.y+m[2][2]*v.z);
}

Aabb::Aabb(const Aabb &b,const Matrix4 &mat)
{
    const Vector3 v[4]=
    {
        Vector3(b.delta.x,b.delta.y,b.delta.z),
        Vector3(b.delta.x,-b.delta.y,b.delta.z),
        Vector3(b.delta.x,b.delta.y,-b.delta.z),
        Vector3(b.delta.x,-b.delta.y,-b.delta.z)
    };
    for(int i=0;i<4;++i)
        delta=Vector3::max(scale_rotate(mat,v[i]).abs(),delta);
    origin = mat * b.origin;
}

bool Aabb::testIntersect(const Vector3 &p) const
{
    RoxMath::Vector3 o_abs=(origin-p).abs();
    return o_abs.x <= delta.x
        && o_abs.y <= delta.y
        && o_abs.z <= delta.z;
}

bool Aabb::testIntersect(const Aabb &box) const
{
    RoxMath::Vector3 o_abs=(box.origin-origin).abs();
    return o_abs.x <= box.delta.x+delta.x
        && o_abs.y <= box.delta.y+delta.y
        && o_abs.z <= box.delta.z+delta.z;
}

}
