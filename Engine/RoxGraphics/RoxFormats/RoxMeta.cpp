// nya-engine (C) nyan.developer@gmail.com released under the MIT license (see LICENSE)

#include "RoxMeta.h"
#include "RoxMemory/RoxMemoryReader.h"
#include "RoxMemory/RoxMemoryWriter.h"
#include "RoxResources/RoxResources.h"
#include <cstring> // Replaced <string.h> with <cstring>

namespace RoxFormats
{

    static const char* nya_meta = "nya_meta"; // Variable names remain unchanged
    using uint = unsigned int; // Replaced typedef with using

    bool Meta::read(const void* data, std::size_t size)
    {
        *this = Meta();

        if (!data || size < 128)
            return false;

        RoxMemory::RoxMemoryReader r(data, size);
        if (!r.test(nya_meta, std::strlen(nya_meta)))
        {
            // Check at the end

            if (!r.seek(size - (std::strlen(nya_meta) + sizeof(uint))))
                return false;

            if (!r.test(nya_meta, std::strlen(nya_meta)))
                return false;

            const uint meta_size = r.read<uint>();
            if (!r.seek(size - meta_size))
                return false;

            return read(r.getData(), r.getRemained() - (std::strlen(nya_meta) + sizeof(uint)));
        }

        const uint count = r.read<uint>();
        values.resize(count);
        for (uint i = 0; i < count; ++i)
        {
            values[i].first = r.readString();
            values[i].second = r.readString();
        }

        return true;
    }

    std::size_t Meta::write(void* data, std::size_t size) const
    {
        RoxMemory::RoxMemoryWriter writer(data, size);
        writer.write(nya_meta, std::strlen(nya_meta));
        writer.writeUint(static_cast<uint>(values.size()));
        for (uint i = 0; i < static_cast<uint>(values.size()); ++i)
        {
            writer.writeString(values[i].first);
            writer.writeString(values[i].second);
        }

        writer.write(nya_meta, std::strlen(nya_meta));
        writer.writeUint(static_cast<unsigned int>(writer.getOffset() + sizeof(uint)));
        return writer.getOffset();
    }

} // namespace RoxFormats
