//nya-engine (C) nyan.developer@gmail.com released under the MIT license (see LICENSE)

#include "meta.h"
#include "memory/memory_reader.h"
#include "memory/memory_writer.h"
#include "resources/resources.h"

namespace nya_formats
{

static const char *nya_meta="nya_meta";
typedef unsigned int uint;

bool meta::read(const void *data,size_t size)
{
    nya_memory::memory_reader r(data,size);
    if(!r.test(nya_meta,strlen(nya_meta)))
    {
        //check at the end

        if(!r.seek(size-(strlen(nya_meta)+sizeof(uint))))
            return false;

        if(!r.test(nya_meta,strlen(nya_meta)))
            return false;

        const uint meta_size=r.read<uint>();
        if(!r.seek(size-meta_size))
           return false;

        return read(r.get_data(),r.get_remained()-(strlen(nya_meta)+sizeof(uint)));
    }

    const uint count=r.read<uint>();
    values.resize(count);
    for(uint i=0;i<count;++i)
        values[i].first=r.read_string(),values[i].second=r.read_string();

    return true;
}

size_t meta::write(void *data,size_t size) const
{
    nya_memory::memory_writer writer(data,size);
    writer.write(nya_meta,strlen(nya_meta));
    writer.write_uint((uint)values.size());
    for(uint i=0;i<(uint)values.size();++i)
        writer.write_string(values[i].first),writer.write_string(values[i].second);

    writer.write(nya_meta,strlen(nya_meta));
    writer.write_uint((unsigned int)(writer.get_offset()+sizeof(uint)));
    return writer.get_offset();
}

}
