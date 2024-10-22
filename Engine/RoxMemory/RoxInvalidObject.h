//nya-engine (C) nyan.developer@gmail.com released under the MIT license (see LICENSE)

#pragma once

namespace RoxMemory
{

template<class t>
t &invalidObject()
{
    static t invalidObject;
    invalidObject.~t();
    new (&invalidObject) t();
    return invalidObject;
}

}
