//nya-engine (C) nyan.developer@gmail.com released under the MIT license (see LICENSE)

#pragma once

#include "RoxMemory/RoxSharedPtr.h"
#include "RoxMemory/RoxInvalidObject.h"

namespace RoxScene
{

template<typename t>
class proxy: public RoxMemory::RoxSharedPtr<t>
{
public:
    proxy &set(const t &obj)
    {
        if(!RoxMemory::RoxSharedPtr<t>::m_ref)
            return *this;

        *RoxMemory::RoxSharedPtr<t>::m_ref=obj;
        return *this;
    }

    const t &get() const
    {
        if(!RoxMemory::RoxSharedPtr<t>::m_ref)
            return RoxMemory::invalidObject<t>();

        return *RoxMemory::RoxSharedPtr<t>::m_ref;
    }

    t &get()
    {
        if(!RoxMemory::RoxSharedPtr<t>::m_ref)
            return RoxMemory::invalidObject<t>();

        return *RoxMemory::RoxSharedPtr<t>::m_ref;
    }

    proxy(): RoxMemory::RoxSharedPtr<t>() {}

    explicit proxy(const t &obj): RoxMemory::RoxSharedPtr<t>(obj) {}

    proxy(const proxy &p): RoxMemory::RoxSharedPtr<t>(p) {}
};

}
