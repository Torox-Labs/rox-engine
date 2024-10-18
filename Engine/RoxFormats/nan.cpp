//nya-engine (C) nyan.developer@gmail.com released under the MIT license (see LICENSE)

#include "nan.h"
#include "memory/memory_reader.h"
#include "memory/memory_writer.h"
#include <stdio.h>
#include <stdint.h>

namespace { const char nan_sign[]={'n','y','a',' ','a','n','i','m'}; }

namespace nya_formats
{

void read_frame(nan::pos_vec3_linear_frame &f,nya_memory::memory_reader &reader)
{
    f.time=reader.read<uint32_t>();
    f.pos=reader.read<nya_math::vec3>();
}

void write_frame(const nan::pos_vec3_linear_frame &f,nya_memory::memory_writer &writer)
{
    writer.write_uint(f.time);
    writer.write_float(f.pos.x);
    writer.write_float(f.pos.y);
    writer.write_float(f.pos.z);
}

void read_frame(nan::rot_quat_linear_frame &f,nya_memory::memory_reader &reader)
{
    f.time=reader.read<uint32_t>();
    f.rot=reader.read<nya_math::quat>();
}

void write_frame(const nan::rot_quat_linear_frame &f,nya_memory::memory_writer &writer)
{
    writer.write_uint(f.time);
    writer.write_float(f.rot.v.x);
    writer.write_float(f.rot.v.y);
    writer.write_float(f.rot.v.z);
    writer.write_float(f.rot.w);
}

void read_frame(nan::float_linear_frame &f,nya_memory::memory_reader &reader)
{
    f.time=reader.read<uint32_t>();
    f.value=reader.read<float>();
}

void write_frame(const nan::float_linear_frame &f,nya_memory::memory_writer &writer)
{
    writer.write_uint(f.time);
    writer.write_float(f.value);
}

template <typename t> void read_curves(std::vector<nan::curve<t> > &array,nya_memory::memory_reader &reader)
{
    const uint32_t curves_count=reader.read<uint32_t>();
    array.resize(curves_count);
    for(uint32_t i=0;i<curves_count;++i)
    {
        array[i].bone_name=reader.read_string();
        const uint32_t frames_count=reader.read<uint32_t>();
        array[i].frames.resize(frames_count);
        for(uint32_t j=0;j<frames_count;++j)
            read_frame(array[i].frames[j],reader);
    }
}

template <typename t> void write_curves(const std::vector<nan::curve<t> > &array,nya_memory::memory_writer &writer)
{
    const uint32_t curves_count=uint32_t(array.size());
    writer.write_uint(curves_count);
    for(uint32_t i=0;i<curves_count;++i)
    {
        writer.write_string(array[i].bone_name);
        const uint32_t frames_count=uint32_t(array[i].frames.size());
        writer.write_uint(frames_count);
        for(uint32_t j=0;j<frames_count;++j)
            write_frame(array[i].frames[j],writer);
    }
}

bool nan::read(const void *data,size_t size)
{
    *this=nan();

    if(!data || !size)
        return false;

    nya_memory::memory_reader reader(data,size);

    if(!reader.test(nan_sign,sizeof(nan_sign)))
        return false;

    version=reader.read<uint32_t>();
    if(version!=1)
        return false;

    read_curves(pos_vec3_linear_curves,reader);
    read_curves(rot_quat_linear_curves,reader);
    read_curves(float_linear_curves,reader);

    return true;
}

size_t nan::write(void *data,size_t size) const
{
    nya_memory::memory_writer writer(data,size);
    writer.write(nan_sign,sizeof(nan_sign));
    writer.write_uint(version);

    write_curves(pos_vec3_linear_curves,writer);
    write_curves(rot_quat_linear_curves,writer);
    write_curves(float_linear_curves,writer);

    return writer.get_offset();
}

}
