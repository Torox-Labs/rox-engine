//nya-engine (C) nyan.developer@gmail.com released under the MIT license (see LICENSE)

#pragma once

namespace nya_memory
{

class non_copyable
{
protected:
    non_copyable() {}

private:
    non_copyable(const non_copyable& );
    non_copyable& operator=(const non_copyable& );
};

}
