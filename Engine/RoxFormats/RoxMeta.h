// nya-engine (C) nyan.developer@gmail.com released under the MIT license (see LICENSE)

#pragma once

#include <vector>
#include <string>
#include <cstddef>

namespace RoxFormats
{

    struct Meta
    {
        std::vector<std::pair<std::string, std::string>> values;

    public:
        bool read(const void* data, std::size_t size);
        std::size_t write(void* data, std::size_t size) const; // to_size = getSize()
        std::size_t getSize() const { return write(nullptr, 0); }
    };

} // namespace RoxFormats
