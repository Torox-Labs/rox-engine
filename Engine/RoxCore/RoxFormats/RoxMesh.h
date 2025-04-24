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

#pragma once

#include <vector>
#include <string>
#include <cstddef>

#include "RoxMath/RoxVector.h"
#include "RoxMath/RoxQuaternion.h"

namespace RoxFormats
{

    struct Mesh
    {
        unsigned int version;

        struct ChunkInfo
        {
            unsigned int type;
            unsigned int size;
            const void* data;
        };

        enum SectionType
        {
            MESH_DATA,
            SKELETON,
            MATERIALS,
            GENERAL
        };

        std::vector<ChunkInfo> chunks;

        Mesh() : version(0) {}

    public:
        bool readChunksInfo(const void* data, std::size_t size);

    public:
        struct Header
        {
            unsigned int version;
            unsigned int chunksCount;
        };

        static std::size_t readHeader(Header& outHeader, const void* data, std::size_t size = Mesh::header_size);
        static std::size_t readChunkInfo(ChunkInfo& outChunkInfo, const void* data, std::size_t size);

    public:
        std::size_t getMeshSize() const;
        std::size_t writeToBuffer(void* toData, std::size_t toSize) const; // toSize = getMeshSize()

    public:
        static std::size_t writeHeaderToBuffer(const Header& h, void* toData, std::size_t toSize = Mesh::header_size);
        static std::size_t writeHeaderToBuffer(unsigned int chunksCount, void* toData, std::size_t toSize = Mesh::header_size);

        static std::size_t getChunkWriteSize(std::size_t chunkDataSize);
        static std::size_t writeChunkToBuffer(const ChunkInfo& chunk, void* toData, std::size_t toSize); // toSize = getChunkWriteSize()

    public:
        const static std::size_t header_size = 16;
        const static unsigned int latest_version = 2;
    };

    struct nms_mesh_chunk
    {
        enum el_type
        {
            pos,
            normal,
            color,
            tc0 = 100
        };

        enum vertex_atrib_type
        {
            float16,
            float32,
            uint8
        };

        enum ind_size
        {
            no_indices = 0,
            index2b = 2,
            index4b = 4
        };

        struct element
        {
            unsigned int type;
            unsigned int dimension;
            unsigned int offset;
            vertex_atrib_type data_type;
            std::string semantics;

            element() : type(0), dimension(0), offset(0), data_type(float32) {}
        };

        enum draw_element_type
        {
            triangles,
            triangle_strip,
            points,
            lines,
            line_strip
        };

        struct group
        {
            RoxMath::Vector3 aabb_min;
            RoxMath::Vector3 aabb_max;

            std::string name;
            unsigned int material_idx;
            unsigned int offset;
            unsigned int count;
            draw_element_type element_type;

            group() : material_idx(0), offset(0), count(0), element_type(triangles) {}
        };

        struct lod { std::vector<group> groups; };

        RoxMath::Vector3 aabb_min;
        RoxMath::Vector3 aabb_max;

        std::vector<element> elements;
        unsigned int verts_count;
        unsigned int vertex_stride;
        const void* vertices_data;

        ind_size index_size;
        unsigned int indices_count;
        const void* indices_data;

        std::vector<lod> lods;

    public:
        nms_mesh_chunk() : verts_count(0), vertex_stride(0), vertices_data(0),
            index_size(no_indices), indices_count(0), indices_data(0) {}
    public:
        size_t read_header(const void* data, size_t size, int version); //0 if invalid

    public:
        size_t get_chunk_size() const { return write_to_buf(0, 0); }
        size_t write_to_buf(void* to_data, size_t to_size) const;
    };

    struct nms_material_chunk
    {
        struct texture_info
        {
            std::string semantics;
            std::string filename;
        };

        struct string_param
        {
            std::string name;
            std::string value;
        };

        struct vector_param
        {
            std::string name;
            RoxMath::Vector4 value;
        };

        struct int_param
        {
            std::string name;
            int value;

            int_param() : value(0) {}
        };

        struct material_info
        {
            std::string name;
            std::vector<texture_info> textures;
            std::vector<string_param> strings;
            std::vector<vector_param> vectors;
            std::vector<int_param> ints;

        public:
            void add_texture_info(const char* semantics, const char* filename, bool unique = true);
            void add_string_param(const char* name, const char* value, bool unique = true);
            void add_vector_param(const char* name, const RoxMath::Vector4& value, bool unique = true);
            void add_int_param(const char* name, int value, bool unique = true);
        };

        std::vector<material_info> materials;

    public:
        bool read(const void* data, size_t size, int version);

    public:
        size_t get_chunk_size() const { return write_to_buf(0, 0); }
        size_t write_to_buf(void* to_data, size_t to_size) const;
    };

    struct nms_skeleton_chunk
    {
        struct bone
        {
            std::string name;
            RoxMath::Quaternion rot;
            RoxMath::Vector3 pos;
            int parent;

            bone() : parent(-1) {}
        };

        std::vector<bone> bones;

    public:
        void sort();
        int get_bone_idx(const char* name) const;
        bone& add_bone(const char* name);

    public:
        bool read(const void* data, size_t size, int version);

    public:
        size_t get_chunk_size() const { return write_to_buf(0, 0); }
        size_t write_to_buf(void* to_data, size_t to_size) const;
    };

    struct nms_general_chunk
    {
        struct string_param
        {
            std::string name;
            std::string value;
        };

        struct vector_param
        {
            std::string name;
            RoxMath::Vector4 value;
        };

        struct object
        {
            std::string name;
            std::string type;
            std::vector<string_param> strings;
            std::vector<vector_param> vectors;

            void add_string_param(const char* name, const char* value, bool unique = true);
            void add_vector_param(const char* name, const RoxMath::Vector4& value, bool unique = true);
        };

        std::vector<object> objects;

    public:
        bool read(const void* data, size_t size, int version);

    public:
        size_t get_chunk_size() const { return write_to_buf(0, 0); }
        size_t write_to_buf(void* to_data, size_t to_size) const;
    };


}
