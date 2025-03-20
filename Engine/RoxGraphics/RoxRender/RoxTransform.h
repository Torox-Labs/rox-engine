// Updated By the ROX_ENGINE
// Copyright © 2024 Torox Project
// Portions Copyright © 2013 nyan.developer@gmail.com (nya-engine)
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

#include "RoxMath/RoxMatrix.h"

namespace RoxRender
{

class RoxTransform
{
public:
    void setProjectionMatrix(const RoxMath::Matrix4 &mat);
    void setModelviewMatrix(const RoxMath::Matrix4 &mat);
    void setOrientationMatrix(const RoxMath::Matrix4 &mat);

public:
    const RoxMath::Matrix4 &getProjectionMatrix() const { return m_has_orientation?m_orientated_proj:m_projection; }
    const RoxMath::Matrix4 &getModelviewMatrix() const { return m_modelview; }
    const RoxMath::Matrix4 &getModelviewprojectionMatrix() const;

    const RoxMath::Matrix4 &getOrientationMatrix() const { return m_orientation; }
    bool hasOrientationMatrix() const { return m_has_orientation; }

public:
    static RoxTransform&get()
    {
        static RoxTransform tr;
        return tr;
    }

    RoxTransform(): m_has_orientation(false),m_recalc_mvp(false) {}

private:
    RoxMath::Matrix4 m_projection;
    RoxMath::Matrix4 m_modelview;

    RoxMath::Matrix4 m_orientation;
    RoxMath::Matrix4 m_orientated_proj;

    mutable RoxMath::Matrix4 m_modelviewproj;

    bool m_has_orientation;
    mutable bool m_recalc_mvp;
};

}
