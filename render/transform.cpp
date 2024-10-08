//nya-engine (C) nyan.developer@gmail.com released under the MIT license (see LICENSE)

#include "transform.h"

namespace nya_render
{

void transform::set_orientation_matrix(const nya_math::mat4 &mat)
{
    m_orientation=mat;
    m_has_orientation=true;
    set_projection_matrix(m_projection);
}

void transform::set_projection_matrix(const nya_math::mat4 &mat)
{
    m_projection=mat, m_recalc_mvp=true;
    if(m_has_orientation)
        m_orientated_proj=m_projection*m_orientation;
}

void transform::set_modelview_matrix(const nya_math::mat4 &mat) { m_modelview=mat, m_recalc_mvp=true; }

const nya_math::mat4 &transform::get_modelviewprojection_matrix() const
{
    if(!m_recalc_mvp)
        return m_modelviewproj;

    m_modelviewproj=m_modelview*get_projection_matrix();
    m_recalc_mvp=false;

    return m_modelviewproj;
}

}
