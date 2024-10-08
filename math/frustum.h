//nya-engine (C) nyan.developer@gmail.com released under the MIT license (see LICENSE)

#pragma once

#include "aabb.h"

namespace nya_math
{

class frustum
{
public:
    bool test_intersect(const aabb &box) const;
    bool test_intersect(const vec3 &v) const;

public:
    frustum() {}
    frustum(const mat4 &m);

private:
    struct plane
    {
        vec3 n;
        vec3 abs_n;
        float d;

        plane(): d(0) {}
    };

    plane m_planes[6];
};

}
