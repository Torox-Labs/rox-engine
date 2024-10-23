// nya-engine (C) nyan.developer@gmail.com released under the MIT license (see LICENSE)

#pragma once

#include "RoxMemory/RoxSharedPtr.h"
#include "RoxMemory/RoxInvalidObject.h"

namespace RoxScene
{

    template<typename T>
    class RoxProxy : public RoxMemory::RoxSharedPtr<T>
    {
    public:
        RoxProxy& set(const T& obj)
        {
            if (!this->m_ref)
                return *this;

            *(this->m_ref) = obj;
            return *this;
        }

        const T& get() const
        {
            if (!this->m_ref)
                return RoxMemory::invalidObject<T>();

            return *(this->m_ref);
        }

        T& get()
        {
            if (!this->m_ref)
                return RoxMemory::invalidObject<T>();

            return *(this->m_ref);
        }

        RProxy() : RoxMemory::RoxSharedPtr<T>() {}

        explicit RProxy(const T& obj) : RoxMemory::RoxSharedPtr<T>(obj) {}

        RProxy(const RProxy& p) : RoxMemory::RoxSharedPtr<T>(p) {}
    };

} // namespace RoxScene
