//nya-engine (C) nyan.developer@gmail.com released under the MIT license (see LICENSE)

#pragma once

namespace nya_memory
{

template<class t>
t &invalid_object()
{
    static t invalid_object;
    invalid_object.~t();
    new (&invalid_object) t();
    return invalid_object;
}

}
