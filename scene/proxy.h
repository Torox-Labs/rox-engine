//nya-engine (C) nyan.developer@gmail.com released under the MIT license (see LICENSE)

#pragma once

#include "memory/shared_ptr.h"
#include "memory/invalid_object.h"

namespace nya_scene
{

template<typename t>
class proxy: public nya_memory::shared_ptr<t>
{
public:
    proxy &set(const t &obj)
    {
        if(!nya_memory::shared_ptr<t>::m_ref)
            return *this;

        *nya_memory::shared_ptr<t>::m_ref=obj;
        return *this;
    }

    const t &get() const
    {
        if(!nya_memory::shared_ptr<t>::m_ref)
            return nya_memory::invalid_object<t>();

        return *nya_memory::shared_ptr<t>::m_ref;
    }

    t &get()
    {
        if(!nya_memory::shared_ptr<t>::m_ref)
            return nya_memory::invalid_object<t>();

        return *nya_memory::shared_ptr<t>::m_ref;
    }

    proxy(): nya_memory::shared_ptr<t>() {}

    explicit proxy(const t &obj): nya_memory::shared_ptr<t>(obj) {}

    proxy(const proxy &p): nya_memory::shared_ptr<t>(p) {}
};

}
