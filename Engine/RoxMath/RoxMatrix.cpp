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

#include "RoxMatrix.h"
#include "RoxVector.h"
#include "RoxQuaternion.h"

namespace RoxMath
{

Matrix4 &Matrix4::identity()
{
    for(int i=0;i<4;++i)
    for(int j=0;j<4;++j)
        m[i][j]=0.0f;

    m[0][0]=m[1][1]=m[2][2]=m[3][3]=1.0f;

    return *this;
}

Matrix4 &Matrix4::translate(float x,float y,float z)
{
    for(int i=0;i<4;++i)
        m[3][i]+=m[0][i]*x+m[1][i]*y+m[2][i]*z;

    return *this;
}

Matrix4 &Matrix4::translate(const Vector3 &v)
{
    for(int i=0;i<4;++i)
        m[3][i]+=m[0][i]*v.x+m[1][i]*v.y+m[2][i]*v.z;
        
    return *this;
}

Matrix4 &Matrix4::rotate(AngleDeg angle,float x,float y,float z)
{
    const float mag=sqrtf(x*x+y*y+z*z);
    const float sin_a=-sin(angle);
    const float cos_a=cos(angle);

    if(mag < 0.001f)
        return *this;

    const float one_minus_cos=1.0f-cos_a;

    float xx, yy, zz, xy, yz, zx, xs, ys, zs;

    x/=mag; y/=mag; z/=mag;
    xx=x*x; yy=y*y; zz=z*z;
    xy=x*y; yz=y*z; zx=z*x;
    xs=x*sin_a; ys=y*sin_a; zs=z*sin_a;

    Matrix4 rot;

    rot[0][0]=one_minus_cos*xx+cos_a;
    rot[0][1]=one_minus_cos*xy-zs;
    rot[0][2]=one_minus_cos*zx+ys;
    rot[0][3]=0;

    rot[1][0]=one_minus_cos*xy+zs;
    rot[1][1]=one_minus_cos*yy+cos_a;
    rot[1][2]=one_minus_cos*yz-xs;
    rot[1][3]=0;

    rot[2][0]=one_minus_cos*zx-ys;
    rot[2][1]=one_minus_cos*yz+xs;
    rot[2][2]=one_minus_cos*zz+cos_a;
    rot[2][3]=0;

    rot[3][0]=0;
    rot[3][1]=0;
    rot[3][2]=0;
    rot[3][3]=1.0f;

    return *this=rot*(*this);
}

Matrix4 &Matrix4::rotate(AngleDeg angle,const Vector3 &v)
{
    return rotate(angle,v.x,v.y,v.z);
}

Matrix4 &Matrix4::rotate(const Quaternion &q)
{
    return *this=Matrix4(q)*(*this);
}

Matrix4 &Matrix4::perspective(AngleDeg fov,float aspect,float near,float far)
{
	const float h=tan(fov*0.5f)*near;
    const float w=h*aspect;

    return frustrum(-w,w,-h,h,near,far);
}

Matrix4 &Matrix4::frustrum(float left,float right,float bottom,float top,float near,float far)
{
    if(near<=0 || far<=0)
        return identity();

    const float dx=right-left;
    const float dy=top-bottom;
    const float dz=far-near;

    if(dx<=0 || dy<=0 || dz<=0)
        return identity();

    Matrix4 frust;

    frust[0][0]=2.0f*near/dx;
    frust[1][1]=2.0f*near/dy;

    frust[2][0]=(right+left)/dx;
    frust[2][1]=(top+bottom)/dy;
    frust[2][2]= -(near+far)/dz;
    frust[2][3]= -1.0f;

    frust[3][2]= -2.0f*near*far/dz;
    frust[3][3]=0;

    return *this=frust*(*this);
}

Matrix4 &Matrix4::ortho(float left,float right,float bottom,float top,float near,float far)
{
    const float dx=right-left;
    const float dy=top-bottom;
    const float dz=far-near;

    const float eps=0.01f;
    if(fabsf(dx)<eps || fabsf(dy)<eps || fabsf(dz)<eps)
        return identity();

    Matrix4 ortho;

    ortho[0][0]=2.0f/dx;
    ortho[1][1]=2.0f/dy;
    ortho[2][2]= -2.0f/dz;

    ortho[3][0]= -(right+left)/dx;
    ortho[3][1]= -(top+bottom)/dy;
    ortho[3][2]= -(far+near)/dz;

    return *this=ortho*(*this);
}

inline float get_cofactor(float m0,float m1,float m2,float m3,float m4,float m5,float m6,float m7,float m8)
{
    return m0*(m4*m8 - m5*m7) - m1*(m3*m8 - m5*m6) + m2*(m3*m7 - m4*m6);
}

Matrix4 &Matrix4::invert()
{
    const float c00=get_cofactor(m[1][1],m[1][2],m[1][3],m[2][1],m[2][2],m[2][3],m[3][1],m[3][2],m[3][3]);
    const float c10=get_cofactor(m[1][0],m[1][2],m[1][3],m[2][0],m[2][2],m[2][3],m[3][0],m[3][2],m[3][3]);
    const float c20=get_cofactor(m[1][0],m[1][1],m[1][3],m[2][0],m[2][1],m[2][3],m[3][0],m[3][1],m[3][3]);
    const float c30=get_cofactor(m[1][0],m[1][1],m[1][2],m[2][0],m[2][1],m[2][2],m[3][0],m[3][1],m[3][2]);

    const float d=m[0][0]*c00 - m[0][1]*c10 + m[0][2]*c20 - m[0][3]*c30;
    if(fabs(d)<0.00001f)
        return identity();

    const float c01=get_cofactor(m[0][1],m[0][2],m[0][3],m[2][1],m[2][2],m[2][3],m[3][1],m[3][2],m[3][3]);
    const float c11=get_cofactor(m[0][0],m[0][2],m[0][3],m[2][0],m[2][2],m[2][3],m[3][0],m[3][2],m[3][3]);
    const float c21=get_cofactor(m[0][0],m[0][1],m[0][3],m[2][0],m[2][1],m[2][3],m[3][0],m[3][1],m[3][3]);
    const float c31=get_cofactor(m[0][0],m[0][1],m[0][2],m[2][0],m[2][1],m[2][2],m[3][0],m[3][1],m[3][2]);

    const float c02=get_cofactor(m[0][1],m[0][2],m[0][3],m[1][1],m[1][2],m[1][3],m[3][1],m[3][2],m[3][3]);
    const float c12=get_cofactor(m[0][0],m[0][2],m[0][3],m[1][0],m[1][2],m[1][3],m[3][0],m[3][2],m[3][3]);
    const float c22=get_cofactor(m[0][0],m[0][1],m[0][3],m[1][0],m[1][1],m[1][3],m[3][0],m[3][1],m[3][3]);
    const float c32=get_cofactor(m[0][0],m[0][1],m[0][2],m[1][0],m[1][1],m[1][2],m[3][0],m[3][1],m[3][2]);

    const float c03=get_cofactor(m[0][1],m[0][2],m[0][3],m[1][1],m[1][2],m[1][3],m[2][1],m[2][2],m[2][3]);
    const float c13=get_cofactor(m[0][0],m[0][2],m[0][3],m[1][0],m[1][2],m[1][3],m[2][0],m[2][2],m[2][3]);
    const float c23=get_cofactor(m[0][0],m[0][1],m[0][3],m[1][0],m[1][1],m[1][3],m[2][0],m[2][1],m[2][3]);
    const float c33=get_cofactor(m[0][0],m[0][1],m[0][2],m[1][0],m[1][1],m[1][2],m[2][0],m[2][1],m[2][2]);

    const float id=1.0f/d;
    m[0][0]=id*c00; m[0][1]= -id*c01; m[0][2]=id*c02; m[0][3]= -id*c03;
    m[1][0]= -id*c10; m[1][1]=id*c11; m[1][2]= -id*c12; m[1][3]=id*c13;
    m[2][0]=id*c20; m[2][1]= -id*c21; m[2][2]=id*c22; m[2][3]= -id*c23;
    m[3][0]= -id*c30; m[3][1]=id*c31; m[3][2]= -id*c32; m[3][3]=id*c33;

    return *this;
}

Matrix4 &Matrix4::transpose()
{
    float temp;
    for(int i=0;i<4;++i)
    for(int j=i+1;j<4;++j)
        temp=m[i][j],m[i][j]=m[j][i],m[j][i]=temp;

    return *this;
}

Quaternion Matrix4::get_rot() const
{
    Quaternion q;

    const float tr=m[0][0]+m[1][1]+m[2][2];
    if(tr>0.0f)
    {
        const float s=2.0f*sqrtf(tr+1.0f);
        q.v.x=(m[1][2]-m[2][1])/s;
        q.v.y=(m[2][0]-m[0][2])/s;
        q.v.z=(m[0][1]-m[1][0])/s;
        q.w=0.25f*s;
    }
    else if((m[0][0]>m[1][1]) && (m[0][0]>m[2][2]))
    {
        const float s=2.0f*sqrtf(m[0][0]-m[1][1]-m[2][2]+1.0f);
        q.v.x=0.25f*s;
        q.v.y=(m[1][0]+m[0][1])/s;
        q.v.z=(m[2][0]+m[0][2])/s;
        q.w=(m[1][2]-m[2][1])/s;
    }
    else if(m[1][1]>m[2][2])
    {
        const float s=2.0f*sqrtf(m[1][1]-m[0][0]-m[2][2]+1.0f);
        q.v.x=(m[1][0]+m[0][1])/s;
        q.v.y=0.25f*s;
        q.v.z=(m[2][1]+m[1][2])/s;
        q.w=(m[2][0]-m[0][2])/s;
    }
    else
    {
        const float s=2.0f*sqrtf(m[2][2]-m[0][0]-m[1][1]+1.0f);
        q.v.x=(m[2][0]+m[0][2])/s;
        q.v.y=(m[2][1]+m[1][2])/s;
        q.v.z=0.25f*s;
        q.w=(m[0][1]-m[1][0])/s;
    }

    return q;
}

Vector3 Matrix4::get_pos() const
{
    return Vector3(m[3]);
}

Matrix4 Matrix4::operator*(const Matrix4 &mat) const
{
    Matrix4 out;
    for(int i=0;i<4;++i)
    {
        for(int j=0;j<4;++j)
            out[i][j]=m[i][0]*mat[0][j] + m[i][1]*mat[1][j] + m[i][2]*mat[2][j] + m[i][3]*mat[3][j];
    }
    return out;
}

Matrix4 &Matrix4::scale(float sx,float sy,float sz)
{
    for(int i=0;i<4;++i)
        m[0][i]*=sx,m[1][i]*=sy,m[2][i]*=sz;

    return *this;
}

Matrix4 &Matrix4::scale(const Vector3 &v)
{
    return scale(v.x,v.y,v.z);
}

Vector3 operator * (const Vector3 &v,const Matrix4 &m)
{
	return Vector3(m[0][0]*v.x+m[0][1]*v.y+m[0][2]*v.z+m[0][3],
				m[1][0]*v.x+m[1][1]*v.y+m[1][2]*v.z+m[1][3],
				m[2][0]*v.x+m[2][1]*v.y+m[2][2]*v.z+m[2][3]);
}

Vector3 operator * (const Matrix4 &m,const Vector3 &v)
{
	return Vector3(m[0][0]*v.x+m[1][0]*v.y+m[2][0]*v.z+m[3][0],
				m[0][1]*v.x+m[1][1]*v.y+m[2][1]*v.z+m[3][1],
				m[0][2]*v.x+m[1][2]*v.y+m[2][2]*v.z+m[3][2]);
}

Vector4 operator * (const Vector4 &v,const Matrix4 &m)
{
    return Vector4(m[0][0]*v.x+m[0][1]*v.y+m[0][2]*v.z+m[0][3]*v.w,
                m[1][0]*v.x+m[1][1]*v.y+m[1][2]*v.z+m[1][3]*v.w,
                m[2][0]*v.x+m[2][1]*v.y+m[2][2]*v.z+m[2][3]*v.w,
                m[3][0]*v.x+m[3][1]*v.y+m[3][2]*v.z+m[3][3]*v.w);
}

Vector4 operator * (const Matrix4 &m,const Vector4 &v)
{
    return Vector4(m[0][0]*v.x+m[1][0]*v.y+m[2][0]*v.z+m[3][0]*v.w,
				m[0][1]*v.x+m[1][1]*v.y+m[2][1]*v.z+m[3][1]*v.w,
				m[0][2]*v.x+m[1][2]*v.y+m[2][2]*v.z+m[3][2]*v.w,
                m[0][3]*v.x+m[1][3]*v.y+m[2][3]*v.z+m[3][3]*v.w);
}

Matrix4::Matrix4(const float (&m)[4][4],bool transpose)
{
    if (transpose)
    {
        for(int i=0;i<4;++i)
            for(int j=0;j<4;++j)
                this->m[i][j]=m[j][i];
    }
    else
    {
        for(int i=0;i<4;++i)
            for(int j=0;j<4;++j)
                this->m[i][j]=m[i][j];
    }
}

Matrix4::Matrix4(const float (&m)[4][3])
{
    for(int i=0;i<4;++i)
        for(int j=0;j<3;++j)
            this->m[i][j]=m[i][j];
    this->m[0][3]=this->m[1][3]=this->m[2][3]=0.0f,this->m[3][3]=1.0f;
}

Matrix4::Matrix4(const float (&m)[3][4])
{
    for(int i=0;i<4;++i)
        for(int j=0;j<3;++j)
            this->m[i][j]=m[j][i];
    this->m[0][3]=this->m[1][3]=this->m[2][3]=0.0f,this->m[3][3]=1.0f;
}

Matrix4::Matrix4(const Quaternion &q)
{
    const float xx=q.v.x*q.v.x;
    const float yy=q.v.y*q.v.y;
    const float zz=q.v.z*q.v.z;
    const float xy=q.v.x*q.v.y;
    const float yz=q.v.y*q.v.z;
    const float xz=q.v.x*q.v.z;
    const float xw=q.v.x*q.w;
    const float yw=q.v.y*q.w;
    const float zw=q.v.z*q.w;

    m[0][0]=1.0f-2.0f*(yy+zz); m[1][0]=2.0f*(xy-zw); m[2][0]=2.0f*(xz+yw);
    m[0][1]=2.0f*(xy+zw); m[1][1]=1.0f-2.0f*(xx+zz); m[2][1]=2.0f*(yz-xw);
    m[0][2]=2.0f*(xz-yw); m[1][2]=2.0f*(yz+xw); m[2][2]=1.0f-2.0f*(xx+yy);

    for(int i=0;i<3;++i)
        m[3][i]=m[i][3]=0.0f;

    m[3][3]=1.0f;
}

Matrix4 &Matrix4::set(const Vector3 &p,const Quaternion &r)
{
    identity();
    translate(p);
    return rotate(r);
}

Matrix4 &Matrix4::set(const Vector3 &p,const Quaternion &r,const Vector3 &s)
{
    identity();
    translate(p);
    rotate(r);
    return scale(s);
}

}
