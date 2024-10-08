//nya-engine (C) nyan.developer@gmail.com released under the MIT license (see LICENSE)

#pragma once

#include "math/matrix.h"

namespace nya_render
{

class transform
{
public:
    void set_projection_matrix(const nya_math::mat4 &mat);
    void set_modelview_matrix(const nya_math::mat4 &mat);
    void set_orientation_matrix(const nya_math::mat4 &mat);

public:
    const nya_math::mat4 &get_projection_matrix() const { return m_has_orientation?m_orientated_proj:m_projection; }
    const nya_math::mat4 &get_modelview_matrix() const { return m_modelview; }
    const nya_math::mat4 &get_modelviewprojection_matrix() const;

    const nya_math::mat4 &get_orientation_matrix() const { return m_orientation; }
    bool has_orientation_matrix() const { return m_has_orientation; }

public:
    static transform &get()
    {
        static transform tr;
        return tr;
    }

    transform(): m_has_orientation(false),m_recalc_mvp(false) {}

private:
    nya_math::mat4 m_projection;
    nya_math::mat4 m_modelview;

    nya_math::mat4 m_orientation;
    nya_math::mat4 m_orientated_proj;

    mutable nya_math::mat4 m_modelviewproj;

    bool m_has_orientation;
    mutable bool m_recalc_mvp;
};

}
