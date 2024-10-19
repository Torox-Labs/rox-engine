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

#include "RoxMatrix.h"
#include "RoxVector.h"

namespace RoxMath
{

struct Aabb
{
    Vector3 origin;
    Vector3 delta;

public:
    float sqDist(const Vector3 &p) const { return (Vector3::abs(origin-p)-delta).lengthSq(); }
    bool testIntersect(const Vector3 &p) const;
    bool testIntersect(const Aabb &box) const;

public:
    Aabb() {}
    Aabb(const Vector3 &Aabb_min,const Vector3 &Aabb_max);
    Aabb(const Aabb &source,const Vector3 &pos,const Quaternion &rot,const Vector3 &scale);
    Aabb(const Aabb &source,const Matrix4 &mat);
};

}
