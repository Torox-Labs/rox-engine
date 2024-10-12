//nya-engine (C) nyan.developer@gmail.com released under the MIT license (see LICENSE)

#pragma once

#include "align_alloc.h"

namespace nya_memory
{

template<typename t>
class shared_ptr
{
    template<typename tt,typename tf> friend shared_ptr<tt> shared_ptr_cast(shared_ptr<tf>& f);
    template<typename tt,typename tf> friend const shared_ptr<tt> shared_ptr_cast(const shared_ptr<tf>& f);

public:
    bool is_valid() const { return m_ref!=0; }

    shared_ptr &create() { return *this=shared_ptr(t()); }
    shared_ptr &create(const t &obj) { return *this=shared_ptr(obj); }

    const t *operator -> () const { return m_ref; };
    t *operator -> () { return m_ref; };

    bool operator == (const shared_ptr &other) const { return other.m_ref==m_ref; }
    bool operator != (const shared_ptr &other) const { return other.m_ref!=m_ref; }

    int get_ref_count() const { return m_ref?*m_ref_count:0; }

    void free()
    {
        if(!m_ref)
            return;

        if(--(*m_ref_count)<=0)
        {
            align_delete(m_ref);
            delete m_ref_count;
        }

        m_ref=0;
    }

    shared_ptr(): m_ref(0) {}

    explicit shared_ptr(const t &obj)
    {
        m_ref=align_new(obj,16);
        m_ref_count=new int(1);
    }

    shared_ptr(const shared_ptr &p)
    {
        m_ref=p.m_ref;
        m_ref_count=p.m_ref_count;
        if(m_ref)
            ++(*m_ref_count);
    }

    shared_ptr &operator=(const shared_ptr &p)
    {
        if(this==&p)
            return *this;

        free();
        m_ref=p.m_ref;
        if(m_ref)
        {
            m_ref_count=p.m_ref_count;
            ++(*m_ref_count);
        }

        return *this;
    }

    ~shared_ptr() { free(); }

protected:
    t *m_ref;
    int *m_ref_count;
};

template<typename to,typename from> shared_ptr<to> shared_ptr_cast(shared_ptr<from>& f)
{
    shared_ptr<to> t;
    t.m_ref=static_cast<to*>(f.m_ref);
    if(f.m_ref) t.m_ref_count=f.m_ref_count, ++(*t.m_ref_count);
    return t;
}

template<typename to,typename from> const shared_ptr<to> shared_ptr_cast(const shared_ptr<from>& f)
{
    shared_ptr<to> t;
    t.m_ref=static_cast<to*>(f.m_ref);
    if(f.m_ref) t.m_ref_count=f.m_ref_count, ++(*t.m_ref_count);
    return t;
}

}
