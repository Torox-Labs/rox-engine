//nya-engine (C) nyan.developer@gmail.com released under the MIT license (see LICENSE)

#pragma once

#include "matrix.h"
#include "vector.h"

namespace nya_math
{

struct aabb
{
    vec3 origin;
    vec3 delta;

public:
    float sq_dist(const vec3 &p) const { return (vec3::abs(origin-p)-delta).length_sq(); }
    bool test_intersect(const vec3 &p) const;
    bool test_intersect(const aabb &box) const;

public:
    aabb() {}
    aabb(const vec3 &aabb_min,const vec3 &aabb_max);
    aabb(const aabb &source,const vec3 &pos,const quat &rot,const vec3 &scale);
    aabb(const aabb &source,const mat4 &mat);
};

}
