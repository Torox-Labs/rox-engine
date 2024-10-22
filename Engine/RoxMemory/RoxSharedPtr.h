//nya-engine (C) nyan.developer@gmail.com released under the MIT license (see LICENSE)

#pragma once

#include "RoxAlignAlloc.h"

namespace RoxMemory
{

template<typename t>
class RoxSharedPtr
{
    template<typename tt,typename tf> friend RoxSharedPtr<tt> shared_ptr_cast(RoxSharedPtr<tf>& f);
    template<typename tt,typename tf> friend const RoxSharedPtr<tt> shared_ptr_cast(const RoxSharedPtr<tf>& f);

public:
    bool isValid() const { return m_ref!=0; }

    RoxSharedPtr &create() { return *this=RoxSharedPtr(t()); }
    RoxSharedPtr &create(const t &obj) { return *this=RoxSharedPtr(obj); }

    const t *operator -> () const { return m_ref; };
    t *operator -> () { return m_ref; };

    bool operator == (const RoxSharedPtr &other) const { return other.m_ref==m_ref; }
    bool operator != (const RoxSharedPtr &other) const { return other.m_ref!=m_ref; }

    int getRefCount() const { return m_ref?*m_ref_count:0; }

    void free()
    {
        if(!m_ref)
            return;

        if(--(*m_ref_count)<=0)
        {
            alignDelete(m_ref);
            delete m_ref_count;
        }

        m_ref=0;
    }

    RoxSharedPtr(): m_ref(0) {}

    explicit RoxSharedPtr(const t &obj)
    {
        m_ref=alignNew(obj,16);
        m_ref_count=new int(1);
    }

    RoxSharedPtr(const RoxSharedPtr &p)
    {
        m_ref=p.m_ref;
        m_ref_count=p.m_ref_count;
        if(m_ref)
            ++(*m_ref_count);
    }

    RoxSharedPtr &operator=(const RoxSharedPtr &p)
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

    ~RoxSharedPtr() { free(); }

protected:
    t *m_ref;
    int *m_ref_count;
};

template<typename to,typename from> RoxSharedPtr<to> sharedPtrCast(RoxSharedPtr<from>& f)
{
    RoxSharedPtr<to> t;
    t.m_ref=static_cast<to*>(f.m_ref);
    if(f.m_ref) t.m_ref_count=f.m_ref_count, ++(*t.m_ref_count);
    return t;
}

template<typename to,typename from> const RoxSharedPtr<to> sharedPtrCast(const RoxSharedPtr<from>& f)
{
    RoxSharedPtr<to> t;
    t.m_ref=static_cast<to*>(f.m_ref);
    if(f.m_ref) t.m_ref_count=f.m_ref_count, ++(*t.m_ref_count);
    return t;
}

}
