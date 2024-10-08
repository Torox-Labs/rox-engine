//nya-engine (C) nyan.developer@gmail.com released under the MIT license (see LICENSE)

#pragma once

// 'insert' and 'get_by_key' are O(log(size)) operations
// 'get_by_idx' is O(1) operation
// 'erase', 'get_idx_for_key' and 'get_key_for_idx' are O(size) operations
// copy construction and assigment are O(size^2) operations

#include <map>
#include <vector>
#include <list>
#include <algorithm>
#include "invalid_object.h"

namespace nya_memory
{

template <class object_t,class key_t=std::string>
class indexed_map
{
public:
    bool insert(const key_t &k,const object_t &obj)
    {
        typename keys_map::iterator key_iter=m_keys.find(k);
        if(key_iter==m_keys.end())
        {
            typename objects_list::iterator object_iter = m_objects.insert(m_objects.end(),obj);
            m_keys.insert(std::make_pair(k,object_iter));
            m_indices.push_back(object_iter);
            return true;
        }
        else
        {
            *(key_iter->second)=obj;
            return false;
        }
    }

    object_t &add(const key_t &k)
    {
        typename keys_map::iterator key_iter=m_keys.find(k);
        if(key_iter==m_keys.end())
        {
            typename objects_list::iterator object_iter = m_objects.insert(m_objects.end(),object_t());
            m_keys.insert(std::make_pair(k,object_iter));
            m_indices.push_back(object_iter);
            return *object_iter;
        }
        else
            return *(key_iter->second);
    }

    bool has_key(const key_t &k) const { return m_keys.find(k)!=m_keys.end(); }
    bool is_empty() const { return m_objects.empty(); }
    int get_size() const { return (int)m_objects.size(); }

    int get_idx_for_key(const key_t &k) const
    {
        typename keys_map::const_iterator key_iter=m_keys.find(k);
        if(key_iter==m_keys.end())
            return -1;

        return (int)get_idx_for_iter(key_iter->second);
    }

    // returns invalid key on bad idx
    key_t get_key_for_idx(size_t idx) const
    {
        if(idx>=m_objects.size())
            return invalid_object<key_t>();

        typename objects_list::const_iterator object_iter=m_indices[idx];
        typename keys_map::const_iterator key_iter=m_keys.begin();
        while(key_iter->second!=object_iter)
            ++key_iter;
        return key_iter->first;
    }

    object_t &get_by_idx(size_t idx)
    {
        if(idx>=m_objects.size())
            return invalid_object<object_t>();

        return *(m_indices[idx]);
    }

    const object_t &get_by_idx(size_t idx) const
    {
        if(idx >= m_objects.size())
            return invalid_object<object_t>();

        return *(m_indices[idx]);
    }

    object_t &get_by_key(const key_t &k)
    {
        typename keys_map::iterator iter=m_keys.find(k);
        if(iter==m_keys.end())
            return invalid_object<object_t>();

        return *(iter->second);
    }

    const object_t &get_by_key(const key_t &k) const
    {
        typename keys_map::const_iterator iter=m_keys.find(k);
        if(iter==m_keys.end())
            return invalid_object<object_t>();

        return *(iter->second);
    }

    void clear()
    {
        m_objects.clear();
        m_keys.clear();
        m_indices.clear();
    }

    bool erase_by_idx(size_t idx)
    {
        if(idx>=m_objects.size())
            return false;

        key_t k=get_key_for_idx(idx);
        m_objects.erase(m_indices[idx]);
        m_indices.erase(m_indices.begin()+idx);
        m_keys.erase(k);
        return true;
    }

    bool erase_by_key(const key_t &k)
    {
        typename keys_map::iterator key_iter=m_keys.find(k);
        if(key_iter==m_keys.end())
            return false;

        size_t idx=get_idx_for_iter(key_iter->second);
        m_objects.erase(m_indices[idx]);
        m_indices.erase(m_indices.begin()+idx);
        m_keys.erase(key_iter);
        return true;
    }

public:
    indexed_map() {}
    indexed_map(const indexed_map &m):
    m_objects(m.m_objects)
    {
        for(typename keys_map::const_iterator it=m.m_keys.begin(); it!=m.m_keys.end(); ++it)
            m_keys.insert(std::make_pair(it->first,find_corressponding_iterator(m.m_objects,it->second)));

        for(typename indices_map::const_iterator it=m.m_indices.begin(); it!=m.m_indices.end(); ++it)
            m_indices.push_back(find_corressponding_iterator(m.m_objects,*it));
    }

    indexed_map &operator=(const indexed_map &m)
    {
        m_objects=m.m_objects;

        m_keys.clear();
        for(typename keys_map::const_iterator it=m.m_keys.begin(); it!=m.m_keys.end(); ++it)
            m_keys.insert(std::make_pair(it->first,find_corressponding_iterator(m.m_objects,it->second)));

        m_indices.clear();
        for(typename indices_map::const_iterator it=m.m_indices.begin();it!=m.m_indices.end(); ++it)
            m_indices.push_back(find_corressponding_iterator(m.m_objects,*it));

        return *this;
    }

private:
    typedef std::list<object_t> objects_list;
    typedef std::map<key_t, typename std::list<object_t>::iterator> keys_map;
    typedef std::vector<typename std::list<object_t>::iterator> indices_map;

    size_t get_idx_for_iter(typename objects_list::const_iterator object_iter) const
    {
        size_t result=0;
        while(m_indices[result]!=object_iter)
            ++result;

        return result;
    }

    typename objects_list::iterator find_corressponding_iterator(const objects_list &another_objects,
                                                                 typename objects_list::const_iterator another_iter)
    {
        typename objects_list::iterator result=m_objects.begin();
        typename objects_list::const_iterator another_finder=another_objects.begin();
        while(another_finder!=another_iter)
        {
            ++another_finder;
            ++result;
        }
        return result;
    }

    objects_list m_objects;
    keys_map m_keys;
    indices_map m_indices;
};

}
