//nya-engine (C) nyan.developer@gmail.com released under the MIT license (see LICENSE)

#pragma once

#include "math/matrix.h"
#include "math/vector.h"
#include "math/quaternion.h"
#include "math/constants.h"
#include "math/frustum.h"

namespace nya_scene
{

class transform
{
public:
    static void set(const transform &tr);
    static const transform &get();

public:
    void set_pos(float x,float y,float z) { m_pos.x=x; m_pos.y=y; m_pos.z=z; }
    void set_pos(const nya_math::vec3 &p) { m_pos=p; }
    void set_rot(const nya_math::quat &q) { m_rot=q; }
    void set_rot(nya_math::angle_deg yaw,nya_math::angle_deg pitch,nya_math::angle_deg roll);
    void set_scale(float sx,float sy,float sz) { m_scale.x=sx; m_scale.y=sy; m_scale.z=sz; }

public:
    const nya_math::vec3 &get_pos() const { return m_pos; }
    const nya_math::quat &get_rot() const { return m_rot; }
    const nya_math::vec3 &get_scale() const { return m_scale; }

public:
    nya_math::vec3 inverse_transform(const nya_math::vec3 &vec) const;
    nya_math::quat inverse_transform(const nya_math::quat &quat) const;
    nya_math::vec3 inverse_rot(const nya_math::vec3 &vec) const;
    nya_math::vec3 inverse_rot_scale(const nya_math::vec3 &vec) const;
    nya_math::vec3 transform_vec(const nya_math::vec3 &vec) const;
    nya_math::quat transform_quat(const nya_math::quat &quat) const;
    nya_math::aabb transform_aabb(const nya_math::aabb &box) const;

public:
    transform():m_scale(nya_math::vec3(1.0f,1.0f,1.0f)){}

private:
    void apply() const;

private:
    nya_math::vec3 m_pos;
    nya_math::quat m_rot;
    nya_math::vec3 m_scale;
};

}
