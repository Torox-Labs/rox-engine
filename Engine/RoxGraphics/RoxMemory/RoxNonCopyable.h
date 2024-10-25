//nya-engine (C) nyan.developer@gmail.com released under the MIT license (see LICENSE)

#pragma once

namespace RoxMemory
{

class RoxNonCopyable
{
protected:
    RoxNonCopyable() {}

private:
    RoxNonCopyable(const RoxNonCopyable& );
    RoxNonCopyable& operator=(const RoxNonCopyable& );
};

}
