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

#include "RoxTransform.h"

namespace RoxRender
{

void RoxTransform::setOrientationMatrix(const RoxMath::Matrix4 &mat)
{
    m_orientation=mat;
    m_has_orientation=true;
    setProjectionMatrix(m_projection);
}

void RoxTransform::setProjectionMatrix(const RoxMath::Matrix4 &mat)
{
    m_projection=mat, m_recalc_mvp=true;
    if(m_has_orientation)
        m_orientated_proj=m_projection*m_orientation;
}

void RoxTransform::setModelviewMatrix(const RoxMath::Matrix4 &mat) { m_modelview=mat, m_recalc_mvp=true; }

const RoxMath::Matrix4 & RoxTransform::getModelviewprojectionMatrix() const
{
    if(!m_recalc_mvp)
        return m_modelviewproj;

    m_modelviewproj=m_modelview*getProjectionMatrix();
    m_recalc_mvp=false;

    return m_modelviewproj;
}

}
