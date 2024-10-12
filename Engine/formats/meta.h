//nya-engine (C) nyan.developer@gmail.com released under the MIT license (see LICENSE)

#pragma once

#include <vector>
#include <string>
#include <stddef.h>

namespace nya_formats
{

struct meta
{
    std::vector<std::pair<std::string, std::string> > values;

public:
    bool read(const void *data,size_t size);
    size_t write(void *data,size_t size) const; //to_size=get_size()
    size_t get_size() const { return write(0,0); }
};

}
