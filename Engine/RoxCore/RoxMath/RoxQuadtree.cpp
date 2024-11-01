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

#include "RoxQuadtree.h"

namespace RoxMath
{

Quadtree::quad::quad(const Aabb &box)
{
    x=int(floorf(box.origin.x-box.delta.x));
    z=int(floorf(box.origin.z-box.delta.z));
    size_x=int(ceilf(box.delta.x+box.delta.x));
    size_z=int(ceilf(box.delta.z+box.delta.z));
}

int Quadtree::addObject(const quad &obj,int obj_idx,float min_y,float max_y,const quad &leaf,int leaf_idx,int level)
{
    if(leaf_idx<0)
    {
        leaf_idx=int(m_leaves.size());
        m_leaves.resize(leaf_idx+1);
    }

    if(m_leaves[leaf_idx].min_y>min_y)
        m_leaves[leaf_idx].min_y=min_y;

    if(m_leaves[leaf_idx].max_y<max_y)
        m_leaves[leaf_idx].max_y=max_y;

    if(level<=0)
    {
        m_leaves[leaf_idx].objects.push_back(obj_idx);
        return leaf_idx;
    }

    --level;

    quad child;
    child.size_x=leaf.size_x/2;
    child.size_z=leaf.size_z/2;

    const int center_x=leaf.x+child.size_x;
    const int center_z=leaf.z+child.size_z;

    if(obj.x<=center_x)
    {
        child.x=leaf.x;

        if(obj.z<=center_z)
        {
            child.z=leaf.z;
            const int idx=addObject(obj,obj_idx,min_y,max_y,child,m_leaves[leaf_idx].leaves[0][0],level);
            m_leaves[leaf_idx].leaves[0][0]=idx;
        }

        if(obj.z+obj.size_z>center_z)
        {
            child.z=center_z;
            const int idx=addObject(obj,obj_idx,min_y,max_y,child,m_leaves[leaf_idx].leaves[0][1],level);
            m_leaves[leaf_idx].leaves[0][1]=idx;
        }
    }

    if(obj.x+obj.size_x>center_x)
    {
        child.x=center_x;

        if(obj.z<=center_z)
        {
            child.z=leaf.z;
            const int idx=addObject(obj,obj_idx,min_y,max_y,child,m_leaves[leaf_idx].leaves[1][0],level);
            m_leaves[leaf_idx].leaves[1][0]=idx;
        }

        if(obj.z+obj.size_z>center_z)
        {
            child.z=center_z;
            const int idx=addObject(obj,obj_idx,min_y,max_y,child,m_leaves[leaf_idx].leaves[1][1],level);
            m_leaves[leaf_idx].leaves[1][1]=idx;
        }
    }

    return leaf_idx;
}

void Quadtree::addObject(const Aabb &box,int idx)
{
    if(m_leaves.empty())
        return;

    objects_map::iterator it=m_objects.find(idx);
    if(it!=m_objects.end())
        removeObject(idx);

    m_objects[idx]=box;
    addObject(quad(box),idx,box.origin.y-box.delta.y,box.origin.y+box.delta.y,m_root,0,m_max_level);
}

template<typename t> void removeObject(int obj_idx,int leaf_idx,std::vector<t> &leaves,int parent)
{
    if(leaf_idx<0)
        return;

    t &leaf=leaves[leaf_idx];
    removeObject(obj_idx,leaf.leaves[0][0],leaves,leaf_idx);
    removeObject(obj_idx,leaf.leaves[0][1],leaves,leaf_idx);
    removeObject(obj_idx,leaf.leaves[1][0],leaves,leaf_idx);
    removeObject(obj_idx,leaf.leaves[1][1],leaves,leaf_idx);

    for(int i=0;i<(int)leaf.objects.size();++i)
    {
        if(leaf.objects[i]!=obj_idx)
            continue;

        leaf.objects.erase(leaf.objects.begin()+i);
        break;
    }

    //ToDo: remove leaves
}

void Quadtree::removeObject(int idx)
{
    objects_map::iterator it=m_objects.find(idx);
    if(it==m_objects.end())
        return;

    m_objects.erase(it);
    ::RoxMath::removeObject(idx,0,m_leaves,-1);
}

const Aabb &Quadtree::getObjectAabb(int idx) const
{
    objects_map::const_iterator it=m_objects.find(idx);
    if(it==m_objects.end())
    {
        const static Aabb invalid;
        return invalid;
    }

    return it->second;
}

template<typename s> bool Quadtree::getObjects(s search,const quad &leaf,int leaf_idx,std::vector<int> &result) const
{
    if(leaf_idx<0)
        return false;

    const struct leaf &l=m_leaves[leaf_idx];
    if(!search.checkHeight(l))
        return false;

    if(!l.objects.empty())
    {
        for(int i=0;i<(int)l.objects.size();++i)
        {
            int obj=l.objects[i];
            bool already=false;
            for(int j=0;j<(int)result.size();++j)
            {
                if(result[j]==obj)
                {
                    already=true;
                    break;
                }
            }

            if(!already && search.checkAabb(m_objects.find(obj)->second))
                result.push_back(obj);
        }
        return !result.empty();
    }

    quad child;
    child.size_x=leaf.size_x/2;
    child.size_z=leaf.size_z/2;

    int center_x=leaf.x+child.size_x;
    int center_z=leaf.z+child.size_z;

    if(search.x<=center_x)
    {
        child.x=leaf.x;

        if(search.z<=center_z)
        {
            child.z=leaf.z;
            getObjects(search,child,m_leaves[leaf_idx].leaves[0][0],result);
        }

        if(search.right_z()>center_z)
        {
            child.z=center_z;
            getObjects(search,child,m_leaves[leaf_idx].leaves[0][1],result);
        }
    }

    if(search.right_x()>center_x)
    {
        child.x=center_x;

        if(search.z<=center_z)
        {
            child.z=leaf.z;
            getObjects(search,child,m_leaves[leaf_idx].leaves[1][0],result);
        }

        if(search.right_z()>center_z)
        {
            child.z=center_z;
            getObjects(search,child,m_leaves[leaf_idx].leaves[1][1],result);
        }
    }

    return !result.empty();
}

template<typename o> struct getter_xz
{
    const o &objects;
    getter_xz(const o &objs,int x,int z): objects(objs),x(x),z(z) {}

    int x,z;
    int right_x() const { return x; }
    int right_z() const { return z; }
    template <typename t> bool checkHeight(const t &leaf) const { return true; }
    bool checkAabb(const Aabb &b) const { return fabsf(b.origin.x-x)<=b.delta.x && fabsf(b.origin.z-z)<=b.delta.z; }
};

bool Quadtree::getObjects(int x,int z,std::vector<int> &result) const
{
    if(m_leaves.empty())
        return false;

    result.clear();
    getter_xz<objects_map> search(m_objects,x,z);
    return getObjects(search,m_root,0,result);
}

template<typename o> struct getter_quad: public getter_xz<o>
{
    getter_quad(const o &objs,int x,int z,int size_x,int size_z): getter_xz<o>(objs,x,z) { xr=x+size_x,zr=z+size_z; }

    int xr,zr;
    int right_x() const { return xr; }
    int right_z() const { return zr; }

    bool checkAabb(const Aabb &b) const
    {
        return max(getter_xz<o>::x,int(b.origin.x-b.delta.x)) <= min(xr,int(b.origin.x+b.delta.x)) &&
               max(getter_xz<o>::z,int(b.origin.z-b.delta.z)) <= min(zr,int(b.origin.z+b.delta.z));
    }

    static int min(int a,int b) { return a<b?a:b; }
    static int max(int a,int b) { return a>b?a:b; }
};

bool Quadtree::getObjects(int x,int z,int size_x,int size_z,std::vector<int> &result) const
{
    if(m_leaves.empty())
        return false;

    result.clear();
    getter_quad<objects_map> search(m_objects,x,z,size_x,size_z);
    return getObjects(search,m_root,0,result);
}

template<typename o> struct GetterVector3: public getter_xz<o>
{
    const Vector3 &v;
    GetterVector3(const o &objs,const Vector3 &v): getter_xz<o>(objs,(int)v.x,(int)v.z),v(v) {}
    template <typename t> bool checkHeight(const t &leaf) const { return v.y>=leaf.min_y && v.y<=leaf.max_y; }
    bool checkAabb(const Aabb &b) const { return b.testIntersect(v); }
};

bool Quadtree::getObjects(const Vector3 &v,std::vector<int> &result) const
{
    if(m_leaves.empty())
        return false;

    result.clear();
    GetterVector3<objects_map> search(m_objects,v);
    return getObjects(search,m_root,0,result);
}

template<typename o> struct getter_Aabb: public getter_quad<o>
{
    const Aabb &bbox;
    float min_y,max_y;
    getter_Aabb(const o &objs,const Aabb &b): getter_quad<o>(objs,int(b.origin.x-b.delta.x),int(b.origin.z-b.delta.z),
                                                             int(b.delta.x)*2,int(b.delta.z)*2), bbox(b)
    {
        min_y=b.origin.y-b.delta.y;
        max_y=b.origin.y+b.delta.y;
    }
    
    template <typename t> bool checkHeight(const t &leaf) const { return RoxMath::max(min_y,leaf.min_y) <= RoxMath::min(max_y,leaf.max_y); }
    bool checkAabb(const Aabb &b) const { return b.testIntersect(bbox); }
};

bool Quadtree::getObjects(const Aabb &b, std::vector<int> &result) const
{
    if(m_leaves.empty())
        return false;
    
    result.clear();
    getter_Aabb<objects_map> search(m_objects,b);
    return getObjects(search,m_root,0,result);
}

/*
bool Quadtree::getObjects(const frustum &f, std::vector<int> &result) const
{
    //ToDo
    return false;
}
*/

Quadtree::Quadtree(int x,int z,int size_x,int size_z,int max_level)
{
    m_root.x=x,m_root.z=z,m_root.size_x=size_x,m_root.size_z=size_z;
    m_max_level=max_level;
    m_leaves.resize(1);
}

}
