// Updated By the ROX_ENGINE
// Copyright (C) 2024 Torox Project
// Portions Copyright (C) 2013 nyan.developer@gmail.com (nya-engine)
//
// This file was modified by the Torox Project.
// 
// This file incorporates code from the nya-engine project, which is licensed under the MIT License.
// See the LICENSE-MIT file in the root directory for more information.
//
// This file is also part of the Rox-engine, which is licensed under a dual-license system:
// 1. Free Use License (for non-commercial and commercial use under specific conditions)
// 2. Commercial License (for use on proprietary platforms)
// See the LICENSE file in the root directory for the full Rox-engine license terms.

#include "RoxMesh.h"
#include "RoxMemory/RoxMemoryReader.h"
#include "RoxMemory/RoxMemoryWriter.h"
#include "RoxMemory/RoxInvalidObject.h"

#include <cstdio> 
#include <cstdint>
#include <cstring>

namespace { const char rms_sign[] = { 'n','y','a',' ','m','e','s','h' }; } // Updated signature

namespace RoxFormats
{

    bool Mesh::readChunksInfo(const void* data, std::size_t size)
    {
        *this = Mesh();

        if (!data || !size)
            return false;

        const char* cdata = static_cast<const char*>(data);
        const char* const data_end = cdata + size;
        Header h;
        cdata += readHeader(h, data, size);
        if (cdata == data)
            return false;

        version = h.version;
        chunks.resize(h.chunksCount);
        for (size_t i = 0; i < chunks.size(); ++i)
        {
            cdata += readChunkInfo(chunks[i], cdata, data_end - cdata);
            cdata += chunks[i].size;
            if (cdata > data_end)
            {
                *this = Mesh();
                return false;
            }
        }

        return true;
    }

    std::size_t Mesh::readHeader(Header& outHeader, const void* data, std::size_t size)
    {
        outHeader.version = outHeader.chunksCount = 0;
        if (size < header_size)
            return 0;

        RoxMemory::RoxMemoryReader reader(data, size);
        if (!reader.test(rms_sign, std::strlen(rms_sign)))
            return 0;

        outHeader.version = reader.read<uint32_t>();
        if (!outHeader.version)
            return 0;

        outHeader.chunksCount = reader.read<uint32_t>();

        return reader.getOffset();
    }

    std::size_t Mesh::readChunkInfo(ChunkInfo& outChunkInfo, const void* data, std::size_t size)
    {
        if (size < sizeof(uint32_t) * 2)
            return 0;

        outChunkInfo.type = outChunkInfo.size = 0;
        RoxMemory::RoxMemoryReader reader(data, size);
        outChunkInfo.type = reader.read<uint32_t>();
        outChunkInfo.size = reader.read<uint32_t>();
        outChunkInfo.data = reader.getData();

        return reader.getOffset();
    }

    std::size_t Mesh::getMeshSize() const
    {
        std::size_t size = header_size;
        for (size_t i = 0; i < chunks.size(); ++i)
            size += getChunkWriteSize(chunks[i].size);

        return size;
    }

    std::size_t Mesh::writeToBuffer(void* data, std::size_t size) const
    {
        if (!data)
            return 0;

        char* cdata = static_cast<char*>(data);
        const char* const data_end = cdata + size;

        Header h;
        h.version = version;
        h.chunksCount = static_cast<unsigned int>(chunks.size());
        cdata += writeHeaderToBuffer(h, data, size);
        if (data == cdata)
            return 0;

        for (size_t i = 0; i < chunks.size(); ++i)
            cdata += writeChunkToBuffer(chunks[i], cdata, data_end - cdata);

        return cdata - static_cast<char*>(data);
    }

    std::size_t Mesh::writeHeaderToBuffer(const Header& h, void* toData, std::size_t toSize)
    {
        if (!toData || toSize < header_size)
            return 0;

        RoxMemory::RoxMemoryWriter writer(toData, toSize);
        writer.write(rms_sign, std::strlen(rms_sign));
        writer.writeUint(h.version);
        writer.writeUint(h.chunksCount);

        return writer.getOffset();
    }

    std::size_t Mesh::writeHeaderToBuffer(unsigned int chunksCount, void* toData, std::size_t toSize)
    {
        Header h;
        h.version = latest_version;
        h.chunksCount = chunksCount;
        return writeHeaderToBuffer(h, toData, toSize);
    }

    std::size_t Mesh::getChunkWriteSize(std::size_t chunkDataSize)
    {
        return chunkDataSize + sizeof(uint32_t) * 2;
    }

    std::size_t Mesh::writeChunkToBuffer(const ChunkInfo& chunk, void* toData, std::size_t toSize)
    {
        if (!toData || toSize < getChunkWriteSize(chunk.size))
            return 0;

        RoxMemory::RoxMemoryWriter writer(toData, toSize);
        writer.writeUint(chunk.type);
        writer.writeUint(chunk.size);
        if (!chunk.size)
            return writer.getOffset();

        if (!chunk.data)
            return 0;

        writer.write(chunk.data, chunk.size);
        return writer.getOffset();
    }

    size_t nms_mesh_chunk::read_header(const void* data, size_t size, int version)
    {
        *this = nms_mesh_chunk();

        if (!data || !size)
            return 0;

        typedef uint32_t uint;
        typedef uint16_t ushort;
        typedef uint8_t uchar;

        RoxMemory::RoxMemoryReader reader(data, size);

        aabb_min = reader.read<RoxMath::Vector3>();
        aabb_max = reader.read<RoxMath::Vector3>();

        elements.resize(reader.read<uchar>());
        for (size_t i = 0; i < elements.size(); ++i)
        {
            element& e = elements[i];

            e.offset = vertex_stride;
            e.type = reader.read<uchar>();
            e.dimension = reader.read<uchar>();
            e.data_type = version > 1 ? (vertex_atrib_type)reader.read<uchar>() : float32;
            switch (e.data_type)
            {
            case float16: vertex_stride += e.dimension * 2; break;
            case float32: vertex_stride += e.dimension * 4; break;
            default:
                *this = nms_mesh_chunk();
                return 0;
            };
            e.semantics = reader.readString();
        }

        if (!vertex_stride)
        {
            *this = nms_mesh_chunk();
            return 0;
        }

        verts_count = reader.read<uint>();
        if (!reader.checkRemained(verts_count * vertex_stride))
        {
            *this = nms_mesh_chunk();
            return 0;
        }

        vertices_data = reader.getData();
        if (!reader.skip(verts_count * vertex_stride))
        {
            *this = nms_mesh_chunk();
            return 0;
        }

        const uint index_size = reader.read<uchar>();
        switch (index_size)
        {
        case no_indices: break;

        case index2b:
        case index4b:
            indices_count = reader.read<uint>();
            if (!reader.checkRemained(indices_count * index_size))
            {
                *this = nms_mesh_chunk();
                return 0;
            }

            indices_data = reader.getData();
            if (!reader.skip(index_size * indices_count))
            {
                *this = nms_mesh_chunk();
                return 0;
            }
            break;

        default: return 0;
        }

        this->index_size = (ind_size)index_size;

        lods.resize(reader.read<ushort>());
        for (size_t i = 0; i < lods.size(); ++i)
        {
            lod& l = lods[i];
            l.groups.resize(reader.read<ushort>());
            for (size_t j = 0; j < l.groups.size(); ++j)
            {
                group& g = l.groups[j];
                g.name = reader.readString();

                g.aabb_min = reader.read<RoxMath::Vector3>();
                g.aabb_max = reader.read<RoxMath::Vector3>();

                g.material_idx = reader.read<ushort>();
                g.offset = reader.read<uint>();
                g.count = reader.read<uint>();
                g.element_type = version > 1 ? draw_element_type(reader.read<uchar>()) : triangles;
            }
        }

        return reader.getOffset();
    }

    size_t nms_mesh_chunk::write_to_buf(void* to_data, size_t to_size) const
    {
        RoxMemory::RoxMemoryWriter writer(to_data, to_size);

        writer.write(aabb_min);
        writer.write(aabb_max);

        writer.writeUbyte((unsigned char)elements.size());
        for (size_t i = 0; i < elements.size(); ++i)
        {
            const element& e = elements[i];
            writer.writeUbyte(e.type);
            writer.writeUbyte(e.dimension);
            writer.writeUbyte(e.data_type);
            writer.writeString(e.semantics);
        }

        writer.writeUint(verts_count);
        writer.write(vertices_data, verts_count * vertex_stride);
        writer.writeUbyte(index_size);
        if (index_size)
        {
            writer.writeUint(indices_count);
            writer.write(indices_data, indices_count * index_size);
        }

        writer.writeUshort((unsigned short)lods.size());
        for (size_t i = 0; i < lods.size(); ++i)
        {
            const lod& l = lods[i];
            writer.writeUshort((unsigned short)l.groups.size());
            for (size_t j = 0; j < l.groups.size(); ++j)
            {
                const group& g = l.groups[j];
                writer.writeString(g.name);

                writer.write(g.aabb_min);
                writer.write(g.aabb_max);

                writer.writeUshort(g.material_idx);
                writer.writeUint(g.offset);
                writer.writeUint(g.count);
                writer.writeUbyte(g.element_type);
            }
        }

        return writer.getOffset();
    }

    bool nms_material_chunk::read(const void* data, size_t size, int version)
    {
        *this = nms_material_chunk();

        if (!data || !size)
            return false;

        RoxMemory::RoxMemoryReader reader(data, size);

        materials.resize(reader.read<uint16_t>());
        for (size_t i = 0; i < materials.size(); ++i)
        {
            material_info& m = materials[i];
            m.name = reader.readString();

            m.textures.resize(reader.read<uint16_t>());
            for (size_t j = 0; j < m.textures.size(); ++j)
            {
                m.textures[j].semantics = reader.readString();
                m.textures[j].filename = reader.readString();
            }

            m.strings.resize(reader.read<uint16_t>());
            for (size_t j = 0; j < m.strings.size(); ++j)
            {
                m.strings[j].name = reader.readString();
                m.strings[j].value = reader.readString();
            }

            m.vectors.resize(reader.read<uint16_t>());
            for (size_t j = 0; j < m.vectors.size(); ++j)
            {
                m.vectors[j].name = reader.readString();
                m.vectors[j].value.x = reader.read<float>();
                m.vectors[j].value.y = reader.read<float>();
                m.vectors[j].value.z = reader.read<float>();
                m.vectors[j].value.w = reader.read<float>();
            }

            m.ints.resize(reader.read<uint16_t>());
            for (size_t j = 0; j < m.ints.size(); ++j)
            {
                m.ints[j].name = reader.readString();
                m.ints[j].value = reader.read<int32_t>();
            }
        }

        return true;
    }

    size_t nms_material_chunk::write_to_buf(void* to_data, size_t to_size) const
    {
        RoxMemory::RoxMemoryWriter writer(to_data, to_size);
        writer.writeUshort((unsigned short)materials.size());
        for (size_t i = 0; i < materials.size(); ++i)
        {
            const material_info& m = materials[i];
            writer.writeString(m.name);

            writer.writeUshort((unsigned short)m.textures.size());
            for (size_t j = 0; j < m.textures.size(); ++j)
            {
                writer.writeString(m.textures[j].semantics);
                writer.writeString(m.textures[j].filename);
            }

            writer.writeUshort((unsigned short)m.strings.size());
            for (size_t j = 0; j < m.strings.size(); ++j)
            {
                writer.writeString(m.strings[j].name);
                writer.writeString(m.strings[j].value);
            }

            writer.writeUshort((unsigned short)m.vectors.size());
            for (size_t j = 0; j < m.vectors.size(); ++j)
            {
                writer.writeString(m.vectors[j].name);
                writer.writeFloat(m.vectors[j].value.x);
                writer.writeFloat(m.vectors[j].value.y);
                writer.writeFloat(m.vectors[j].value.z);
                writer.writeFloat(m.vectors[j].value.w);
            }

            writer.writeUshort((unsigned short)m.ints.size());
            for (size_t j = 0; j < m.ints.size(); ++j)
            {
                writer.writeString(m.ints[j].name);
                writer.writeInt(m.ints[j].value);
            }
        }

        return writer.getOffset();
    }

    template<typename t> t& add_param(const char* name, std::vector<t>& array, bool unique)
    {
        if (unique)
        {
            for (size_t i = 0; i < array.size(); ++i)
                if (*((std::string*)&array[i]) == name)
                    return array[i];
        }

        array.resize(array.size() + 1);
        ((std::string*)&array.back())->assign(name);
        return array.back();
    }

    void nms_material_chunk::material_info::add_texture_info(const char* semantics, const char* filename, bool unique)
    {
        if (semantics && filename)
            add_param(semantics, textures, unique).filename.assign(filename);
    }

    void nms_material_chunk::material_info::add_string_param(const char* name, const char* value, bool unique)
    {
        if (name && value) add_param(name, strings, unique).value.assign(value);
    }

    void nms_material_chunk::material_info::add_vector_param(const char* name, const RoxMath::Vector4& value, bool unique)
    {
        if (name) add_param(name, vectors, unique).value = value;
    }

    void nms_material_chunk::material_info::add_int_param(const char* name, int value, bool unique)
    {
        if (name) add_param(name, ints, unique).value = value;
    }

    void nms_skeleton_chunk::sort()
    {
        for (int i = 0; i<int(bones.size()); ++i)
        {
            bool had_sorted = false;
            for (int j = 0; j<int(bones.size()); ++j)
            {
                const int p = bones[j].parent;
                if (p <= j)
                    continue;

                had_sorted = true;
                std::swap(bones[j], bones[p]);
                for (int k = 0; k < (int)bones.size(); ++k)
                {
                    if (bones[k].parent == j)
                        bones[k].parent = p;
                    else if (bones[k].parent == p)
                        bones[k].parent = j;
                }
            }

            if (!had_sorted)
                break;
        }
    }

    int nms_skeleton_chunk::get_bone_idx(const char* name) const
    {
        if (!name)
            return -1;

        for (int i = 0; i < (int)bones.size(); ++i)
        {
            if (bones[i].name == name)
                return i;
        }

        return -1;
    }

    nms_skeleton_chunk::bone& nms_skeleton_chunk::add_bone(const char* name)
    {
        if (!name)
            return RoxMemory::invalidObject<bone>();

        for (size_t i = 0; i < bones.size(); ++i)
        {
            if (bones[i].name == name)
                return bones[i];
        }

        bones.resize(bones.size() + 1);
        bones.back().name = name;
        return bones.back();
    }

    bool nms_skeleton_chunk::read(const void* data, size_t size, int version)
    {
        *this = nms_skeleton_chunk();

        if (!data || !size)
            return false;

        RoxMemory::RoxMemoryReader reader(data, size);

        bones.resize(reader.read<int32_t>());
        for (size_t i = 0; i < bones.size(); ++i)
        {
            bone& b = bones[i];
            b.name = reader.readString();
            b.rot = reader.read<RoxMath::Quaternion>();
            b.pos = reader.read<RoxMath::Vector3>();
            b.parent = reader.read<int32_t>();
        }

        return true;
    }

    size_t nms_skeleton_chunk::write_to_buf(void* to_data, size_t to_size) const
    {
        RoxMemory::RoxMemoryWriter writer(to_data, to_size);
        writer.writeUint((unsigned int)bones.size());
        for (size_t i = 0; i < bones.size(); ++i)
        {
            const bone& b = bones[i];
            writer.writeString(b.name);
            writer.write(b.rot);
            writer.write(b.pos);
            writer.writeInt(b.parent);
        }

        return writer.getOffset();
    }

    void nms_general_chunk::object::add_string_param(const char* name, const char* value, bool unique)
    {
        if (name && value) add_param(name, strings, unique).value.assign(value);
    }

    void nms_general_chunk::object::add_vector_param(const char* name, const RoxMath::Vector4& value, bool unique)
    {
        if (name) add_param(name, vectors, unique).value = value;
    }

    bool nms_general_chunk::read(const void* data, size_t size, int version)
    {
        *this = nms_general_chunk();

        if (!data || !size)
            return false;

        RoxMemory::RoxMemoryReader reader(data, size);

        objects.resize(reader.read<uint16_t>());
        for (size_t i = 0; i < objects.size(); ++i)
        {
            object& o = objects[i];
            o.name = reader.readString();
            o.type = reader.readString();

            o.strings.resize(reader.read<uint16_t>());
            for (size_t j = 0; j < o.strings.size(); ++j)
            {
                o.strings[j].name = reader.readString();
                o.strings[j].value = reader.readString();
            }

            o.vectors.resize(reader.read<uint16_t>());
            for (size_t j = 0; j < o.vectors.size(); ++j)
            {
                o.vectors[j].name = reader.readString();
                o.vectors[j].value.x = reader.read<float>();
                o.vectors[j].value.y = reader.read<float>();
                o.vectors[j].value.z = reader.read<float>();
                o.vectors[j].value.w = reader.read<float>();
            }
        }

        return true;
    }

    size_t nms_general_chunk::write_to_buf(void* to_data, size_t to_size) const
    {
        RoxMemory::RoxMemoryWriter writer(to_data, to_size);
        writer.writeUshort((unsigned short)objects.size());
        for (size_t i = 0; i < objects.size(); ++i)
        {
            const object& o = objects[i];
            writer.writeString(o.name);
            writer.writeString(o.type);

            writer.writeUshort((unsigned short)o.strings.size());
            for (size_t j = 0; j < o.strings.size(); ++j)
            {
                writer.writeString(o.strings[j].name);
                writer.writeString(o.strings[j].value);
            }

            writer.writeUshort((unsigned short)o.vectors.size());
            for (size_t j = 0; j < o.vectors.size(); ++j)
            {
                writer.writeString(o.vectors[j].name);
                writer.writeFloat(o.vectors[j].value.x);
                writer.writeFloat(o.vectors[j].value.y);
                writer.writeFloat(o.vectors[j].value.z);
                writer.writeFloat(o.vectors[j].value.w);
            }
        }

        return writer.getOffset();
    }

} // namespace RoxFormats
