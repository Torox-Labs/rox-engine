//nya-engine (C) nyan.developer@gmail.com released under the MIT license (see LICENSE)

#include "frustum.h"
#include "quaternion.h"

namespace nya_math
{

aabb::aabb(const vec3 &aabb_min,const vec3 &aabb_max)
{
    delta=(aabb_max-aabb_min)*0.5f;
    origin=aabb_min+delta;
}

aabb::aabb(const aabb &b,const vec3 &p,const quat &q,const vec3 &s)
{
    delta = b.delta * s;
    const vec3 v[4]=
    {
        vec3(delta.x,delta.y,delta.z),
        vec3(delta.x,-delta.y,delta.z),
        vec3(delta.x,delta.y,-delta.z),
        vec3(delta.x,-delta.y,-delta.z)
    };
    delta=vec3();
    for(int i=0;i<4;++i)
        delta=vec3::max(q.rotate(v[i]).abs(),delta);
    origin=q.rotate(b.origin * s)+p;
}

inline vec3 scale_rotate(const mat4 &m, const nya_math::vec3 &v)
{
    return vec3(m[0][0]*v.x+m[1][0]*v.y+m[2][0]*v.z,
                m[0][1]*v.x+m[1][1]*v.y+m[2][1]*v.z,
                m[0][2]*v.x+m[1][2]*v.y+m[2][2]*v.z);
}

aabb::aabb(const aabb &b,const mat4 &mat)
{
    const vec3 v[4]=
    {
        vec3(b.delta.x,b.delta.y,b.delta.z),
        vec3(b.delta.x,-b.delta.y,b.delta.z),
        vec3(b.delta.x,b.delta.y,-b.delta.z),
        vec3(b.delta.x,-b.delta.y,-b.delta.z)
    };
    for(int i=0;i<4;++i)
        delta=vec3::max(scale_rotate(mat,v[i]).abs(),delta);
    origin = mat * b.origin;
}

bool aabb::test_intersect(const vec3 &p) const
{
    nya_math::vec3 o_abs=(origin-p).abs();
    return o_abs.x <= delta.x
        && o_abs.y <= delta.y
        && o_abs.z <= delta.z;
}

bool aabb::test_intersect(const aabb &box) const
{
    nya_math::vec3 o_abs=(box.origin-origin).abs();
    return o_abs.x <= box.delta.x+delta.x
        && o_abs.y <= box.delta.y+delta.y
        && o_abs.z <= box.delta.z+delta.z;
}

}
