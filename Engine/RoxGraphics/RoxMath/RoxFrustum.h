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

#include "RoxAabb.h"

namespace RoxMath
{

class RoxFrustum
{
public:
    bool testIntersect(const Aabb &box) const;
    bool testIntersect(const Vector3 &v) const;

public:
    RoxFrustum() {}
    RoxFrustum(const Matrix4 &m);

private:
    struct plane
    {
        Vector3 n;
        Vector3 abs_n;
        float d;

        plane(): d(0) {}
    };

    plane m_planes[6];
};

}
