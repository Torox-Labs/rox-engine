//nya-engine (C) nyan.developer@gmail.com released under the MIT license (see LICENSE)

#include "quaternion.h"
#include "matrix.h"
#include "constants.h"
#include <math.h>

namespace nya_math
{

quat quat::slerp(const quat &q1,const quat &q2,float t)
{
    const float eps=0.001f;
    float scale0,scale1;

    const float cosom=q1.v.dot(q2.v)+q1.w*q2.w;
    if(cosom<0.0f)
    {
        if(1.0f+cosom>eps)
        {
            const float omega=acosf(-cosom);
            const float sinom_inv=1.0f/sinf(omega);
            scale0=sinf((1.0f-t)*omega)*sinom_inv;
            scale1= -sinf(t*omega)*sinom_inv;
        }
        else
        {
            scale0=1.0f-t;
            scale1= -t;
        }
    }
    else
    {
        if(1.0f-cosom>eps)
        {
            const float omega=acosf(cosom);
            const float sinom_inv=1.0f/sinf(omega);
            scale0=sinf((1.0f-t)*omega)*sinom_inv;
            scale1=sinf(t*omega)*sinom_inv;
        }
        else
        {
            scale0=1.0f-t;
            scale1=t;
        }
    }

    return quat(scale0*q1.v.x+scale1*q2.v.x,
                scale0*q1.v.y+scale1*q2.v.y,
                scale0*q1.v.z+scale1*q2.v.z,
                scale0*q1.w+scale1*q2.w);
}

quat quat::nlerp(const quat &q1,const quat &q2,float t)
{
    const float t2=1.0f-t;
    if(q1.v.dot(q2.v)+q1.w*q2.w<0)
        t=-t;

    return quat(t2*q1.v.x+t*q2.v.x,
         t2*q1.v.y+t*q2.v.y,
         t2*q1.v.z+t*q2.v.z,
         t2*q1.w+t*q2.w).normalize();
}

vec3 quat::get_euler() const
{
    const float x2=v.x+v.x;
    const float y2=v.y+v.y;
    const float z2=v.z+v.z;
    const float yz2=v.y*z2;
    const float wx2=w*x2;
    
    float temp=wx2-yz2;
    if(temp>=1.0f)
        temp=1.0f;
    else if(temp<= -1.0f)
        temp= -1.0f;
    
    const float pitch=asinf(temp);
    
    const float yy2=v.y*y2;
    const float xy2=v.x*y2;
    const float zz2=v.z*z2;
    const float wz2=w*z2;
    
    if(pitch>=constants::pi_2)
        return vec3(pitch,atan2f(xy2-wz2,1.0f-yy2-zz2),0.0f);
    
    if(pitch> -constants::pi_2)
    {
        const float xz2=v.x*z2;
        const float wy2=w*y2;
        const float xx2=v.x*x2;
        
        return vec3(pitch,atan2f(xz2+wy2,1.0f-yy2-xx2),
                    atan2f(xy2+wz2,1.0f-xx2-zz2));
    }
    
    return vec3(pitch,-atan2f(xy2-wz2,1.0f-yy2-zz2),0.0f);
}

quat::quat(angle_rad pitch,angle_rad yaw,angle_rad roll)
{
    pitch*=0.5f; yaw*=0.5f; roll*=0.5f;

    const float sin_x=sin(pitch);
    const float cos_x=cos(pitch);

    const float sin_y=sin(yaw);
    const float cos_y=cos(yaw);

    const float sin_z=sin(roll);
    const float cos_z=cos(roll);

    v.x=sin_x*cos_y*cos_z + cos_x*sin_y*sin_z;
    v.y=cos_x*sin_y*cos_z - sin_x*cos_y*sin_z;
    v.z=cos_x*cos_y*sin_z - sin_x*sin_y*cos_z;
    w  =cos_x*cos_y*cos_z + sin_x*sin_y*sin_z;
}

quat::quat(const mat4 &m)
{
    const float t=m[0][0]+m[1][1]+m[2][2];
    if(t>0)
    {
        const float s=0.5f/sqrtf(t+1.0f);
        v.x=(m[1][2]-m[2][1])*s;
        v.y=(m[2][0]-m[0][2])*s;
        v.z=(m[0][1]-m[1][0])*s;
        w=0.25f/s;
    }
    else
    {
        if(m[0][0]>m[1][1] && m[0][0]>m[2][2])
        {
            const float s=2.0f*sqrtf(1.0f+m[0][0]-m[1][1]-m[2][2]);
            const float is=1.0f/s;
            v.x=0.25f*s;
            v.y=(m[1][0]+m[0][1])*is;
            v.z=(m[2][0]+m[0][2])*is;
            w=(m[1][2]-m[2][1])*is;
        }
        else if(m[1][1] > m[2][2])
        {
            const float s=2.0f*sqrtf(1.0f+m[1][1]-m[0][0]-m[2][2]);
            const float is=1.0f/s;
            v.x=(m[1][0]+m[0][1])*is;
            v.y=0.25f*s;
            v.z=(m[2][1]+m[1][2])*is;
            w=(m[2][0]-m[0][2])*is;
        }
        else
        {
            const float s=2.0f*sqrtf(1.0f+m[2][2]-m[0][0]-m[1][1]);
            const float is=1.0f/s;
            v.x=(m[2][0]+m[0][2])*is;
            v.y=(m[2][1]+m[1][2])*is;
            v.z=0.25f*s;
            w=(m[0][1]-m[1][0])*is;
        }
    }
}

quat::quat(const vec3 &axis,angle_rad angle)
{
    angle*=0.5f;

    v=axis*sin(angle);
    w=cos(angle);
}

quat::quat(const vec3 &from,const vec3 &to)
{
    float len=sqrtf(from.length_sq() * to.length_sq());
    if(len<0.0001f)
    {
        w=1.0f;
        return;
    }

    w=sqrtf(0.5f*(1.0f + from.dot(to)/len));
    v=vec3::cross(from,to)/(len*2.0f*w);
}

quat &quat::normalize()
{
    const float len=sqrtf(v.length_sq()+w*w);
    if(len>0.00001f)
    {
        const float len_inv=1.0f/len;
        v*=len_inv;
        w*=len_inv;
    }

    return *this;
}

quat &quat::apply_weight(float weight)
{
    v*=weight;
    w*=weight;
    w+=1.0f-weight;

    return *this;
}

quat quat::operator * (const quat &q) const
{
    return quat(w*q.v.x + v.x*q.w   + v.y*q.v.z - v.z*q.v.y,
                w*q.v.y - v.x*q.v.z + v.y*q.w   + v.z*q.v.x,
                w*q.v.z + v.x*q.v.y - v.y*q.v.x + v.z*q.w,
                w*q.w   - v.x*q.v.x - v.y*q.v.y - v.z*q.v.z);
}

vec3 quat::rotate(const vec3 &vec) const
{
    return vec+vec3::cross(v,vec3::cross(v,vec)+vec*w)*2.0f;
}

vec3 quat::rotate_inv(const vec3 &vec) const
{
    return vec+vec3::cross(vec3::cross(vec,v)+vec*w,v)*2.0f;
}

}
