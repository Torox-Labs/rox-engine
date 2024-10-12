//nya-engine (C) nyan.developer@gmail.com released under the MIT license (see LICENSE)

#include "quadtree.h"
#include <algorithm>

namespace nya_math
{

quadtree::quad::quad(const aabb &box)
{
    x=int(floorf(box.origin.x-box.delta.x));
    z=int(floorf(box.origin.z-box.delta.z));
    size_x=int(ceilf(box.delta.x+box.delta.x));
    size_z=int(ceilf(box.delta.z+box.delta.z));
}

int quadtree::add_object(const quad &obj,int obj_idx,float min_y,float max_y,const quad &leaf,int leaf_idx,int level)
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
            const int idx=add_object(obj,obj_idx,min_y,max_y,child,m_leaves[leaf_idx].leaves[0][0],level);
            m_leaves[leaf_idx].leaves[0][0]=idx;
        }

        if(obj.z+obj.size_z>center_z)
        {
            child.z=center_z;
            const int idx=add_object(obj,obj_idx,min_y,max_y,child,m_leaves[leaf_idx].leaves[0][1],level);
            m_leaves[leaf_idx].leaves[0][1]=idx;
        }
    }

    if(obj.x+obj.size_x>center_x)
    {
        child.x=center_x;

        if(obj.z<=center_z)
        {
            child.z=leaf.z;
            const int idx=add_object(obj,obj_idx,min_y,max_y,child,m_leaves[leaf_idx].leaves[1][0],level);
            m_leaves[leaf_idx].leaves[1][0]=idx;
        }

        if(obj.z+obj.size_z>center_z)
        {
            child.z=center_z;
            const int idx=add_object(obj,obj_idx,min_y,max_y,child,m_leaves[leaf_idx].leaves[1][1],level);
            m_leaves[leaf_idx].leaves[1][1]=idx;
        }
    }

    return leaf_idx;
}

void quadtree::add_object(const aabb &box,int idx)
{
    if(m_leaves.empty())
        return;

    objects_map::iterator it=m_objects.find(idx);
    if(it!=m_objects.end())
        remove_object(idx);

    m_objects[idx]=box;
    add_object(quad(box),idx,box.origin.y-box.delta.y,box.origin.y+box.delta.y,m_root,0,m_max_level);
}

template<typename t> void remove_object(int obj_idx,int leaf_idx,std::vector<t> &leaves,int parent)
{
    if(leaf_idx<0)
        return;

    t &leaf=leaves[leaf_idx];
    remove_object(obj_idx,leaf.leaves[0][0],leaves,leaf_idx);
    remove_object(obj_idx,leaf.leaves[0][1],leaves,leaf_idx);
    remove_object(obj_idx,leaf.leaves[1][0],leaves,leaf_idx);
    remove_object(obj_idx,leaf.leaves[1][1],leaves,leaf_idx);

    for(int i=0;i<(int)leaf.objects.size();++i)
    {
        if(leaf.objects[i]!=obj_idx)
            continue;

        leaf.objects.erase(leaf.objects.begin()+i);
        break;
    }

    //ToDo: remove leaves
}

void quadtree::remove_object(int idx)
{
    objects_map::iterator it=m_objects.find(idx);
    if(it==m_objects.end())
        return;

    m_objects.erase(it);
    ::nya_math::remove_object(idx,0,m_leaves,-1);
}

const aabb &quadtree::get_object_aabb(int idx) const
{
    objects_map::const_iterator it=m_objects.find(idx);
    if(it==m_objects.end())
    {
        const static aabb invalid;
        return invalid;
    }

    return it->second;
}

template<typename s> bool quadtree::get_objects(s search,const quad &leaf,int leaf_idx,std::vector<int> &result) const
{
    if(leaf_idx<0)
        return false;

    const struct leaf &l=m_leaves[leaf_idx];
    if(!search.check_height(l))
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

            if(!already && search.check_aabb(m_objects.find(obj)->second))
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
            get_objects(search,child,m_leaves[leaf_idx].leaves[0][0],result);
        }

        if(search.right_z()>center_z)
        {
            child.z=center_z;
            get_objects(search,child,m_leaves[leaf_idx].leaves[0][1],result);
        }
    }

    if(search.right_x()>center_x)
    {
        child.x=center_x;

        if(search.z<=center_z)
        {
            child.z=leaf.z;
            get_objects(search,child,m_leaves[leaf_idx].leaves[1][0],result);
        }

        if(search.right_z()>center_z)
        {
            child.z=center_z;
            get_objects(search,child,m_leaves[leaf_idx].leaves[1][1],result);
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
    template <typename t> bool check_height(const t &leaf) const { return true; }
    bool check_aabb(const aabb &b) const { return fabsf(b.origin.x-x)<=b.delta.x && fabsf(b.origin.z-z)<=b.delta.z; }
};

bool quadtree::get_objects(int x,int z,std::vector<int> &result) const
{
    if(m_leaves.empty())
        return false;

    result.clear();
    getter_xz<objects_map> search(m_objects,x,z);
    return get_objects(search,m_root,0,result);
}

template<typename o> struct getter_quad: public getter_xz<o>
{
    getter_quad(const o &objs,int x,int z,int size_x,int size_z): getter_xz<o>(objs,x,z) { xr=x+size_x,zr=z+size_z; }

    int xr,zr;
    int right_x() const { return xr; }
    int right_z() const { return zr; }

    bool check_aabb(const aabb &b) const
    {
        return max(getter_xz<o>::x,int(b.origin.x-b.delta.x)) <= min(xr,int(b.origin.x+b.delta.x)) &&
               max(getter_xz<o>::z,int(b.origin.z-b.delta.z)) <= min(zr,int(b.origin.z+b.delta.z));
    }

    static int min(int a,int b) { return a<b?a:b; }
    static int max(int a,int b) { return a>b?a:b; }
};

bool quadtree::get_objects(int x,int z,int size_x,int size_z,std::vector<int> &result) const
{
    if(m_leaves.empty())
        return false;

    result.clear();
    getter_quad<objects_map> search(m_objects,x,z,size_x,size_z);
    return get_objects(search,m_root,0,result);
}

template<typename o> struct getter_vec3: public getter_xz<o>
{
    const vec3 &v;
    getter_vec3(const o &objs,const vec3 &v): getter_xz<o>(objs,(int)v.x,(int)v.z),v(v) {}
    template <typename t> bool check_height(const t &leaf) const { return v.y>=leaf.min_y && v.y<=leaf.max_y; }
    bool check_aabb(const aabb &b) const { return b.test_intersect(v); }
};

bool quadtree::get_objects(const vec3 &v,std::vector<int> &result) const
{
    if(m_leaves.empty())
        return false;

    result.clear();
    getter_vec3<objects_map> search(m_objects,v);
    return get_objects(search,m_root,0,result);
}

template<typename o> struct getter_aabb: public getter_quad<o>
{
    const aabb &bbox;
    float min_y,max_y;
    getter_aabb(const o &objs,const aabb &b): getter_quad<o>(objs,int(b.origin.x-b.delta.x),int(b.origin.z-b.delta.z),
                                                             int(b.delta.x)*2,int(b.delta.z)*2), bbox(b)
    {
        min_y=b.origin.y-b.delta.y;
        max_y=b.origin.y+b.delta.y;
    }
    
    template <typename t> bool check_height(const t &leaf) const { return nya_math::max(min_y,leaf.min_y) <= nya_math::min(max_y,leaf.max_y); }
    bool check_aabb(const aabb &b) const { return b.test_intersect(bbox); }
};

bool quadtree::get_objects(const aabb &b, std::vector<int> &result) const
{
    if(m_leaves.empty())
        return false;
    
    result.clear();
    getter_aabb<objects_map> search(m_objects,b);
    return get_objects(search,m_root,0,result);
}

/*
bool quadtree::get_objects(const frustum &f, std::vector<int> &result) const
{
    //ToDo
    return false;
}
*/

quadtree::quadtree(int x,int z,int size_x,int size_z,int max_level)
{
    m_root.x=x,m_root.z=z,m_root.size_x=size_x,m_root.size_z=size_z;
    m_max_level=max_level;
    m_leaves.resize(1);
}

}
