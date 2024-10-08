//nya-engine (C) nyan.developer@gmail.com released under the MIT license (see LICENSE)

#pragma once

#include <vector>
#include <string>
#include <stddef.h>
#include "math/vector.h"
#include "math/quaternion.h"

namespace nya_formats
{

struct nms
{
    unsigned int version;

    struct chunk_info
    {
        unsigned int type;
        unsigned int size;
        const void *data;
    };

    enum section_type
    {
        mesh_data,
        skeleton,
        materials,
        general
    };

    std::vector<chunk_info> chunks;

    nms(): version(0) {}

public:
    bool read_chunks_info(const void *data,size_t size);

public:
    struct header
    {
        unsigned int version;
        unsigned int chunks_count;
    };

    static size_t read_header(header &out_header,const void *data,size_t size=nms_header_size);
    static size_t read_chunk_info(chunk_info &out_chunk_info,const void *data,size_t size);

public:
    size_t get_nms_size();
    size_t write_to_buf(void *to_data,size_t to_size); //to_size=get_nms_size()

public:
    static size_t write_header_to_buf(unsigned int chunks_count,void *to_data,size_t to_size=nms_header_size)
    {
        header h; h.version=latest_version,h.chunks_count=chunks_count;
        return write_header_to_buf(h,to_data,to_size);
    }

    static size_t write_header_to_buf(const header &h,void *to_data,size_t to_size=nms_header_size);

    static size_t get_chunk_write_size(size_t chunk_data_size);
    static size_t write_chunk_to_buf(const chunk_info &chunk,void *to_data,size_t to_size); //to_size=get_chunk_size()

public:
    const static size_t nms_header_size=16;
    const static unsigned int latest_version=2;
};

struct nms_mesh_chunk
{
    enum el_type
    {
        pos,
        normal,
        color,
        tc0=100
    };

    enum vertex_atrib_type
    {
        float16,
        float32,
        uint8
    };

    enum ind_size
    {
        no_indices=0,
        index2b=2,
        index4b=4
    };

    struct element
    {
        unsigned int type;
        unsigned int dimension;
        unsigned int offset;
        vertex_atrib_type data_type;
        std::string semantics;

        element(): type(0),dimension(0),offset(0),data_type(float32) {}
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
        nya_math::vec3 aabb_min;
        nya_math::vec3 aabb_max;

        std::string name;
        unsigned int material_idx;
        unsigned int offset;
        unsigned int count;
        draw_element_type element_type;

        group(): material_idx(0),offset(0),count(0),element_type(triangles) {}
    };

    struct lod { std::vector<group> groups; };

    nya_math::vec3 aabb_min;
    nya_math::vec3 aabb_max;

    std::vector<element> elements;
    unsigned int verts_count;
    unsigned int vertex_stride;
    const void *vertices_data;

    ind_size index_size;
    unsigned int indices_count;
    const void *indices_data;

    std::vector<lod> lods;

public:
    nms_mesh_chunk(): verts_count(0),vertex_stride(0),vertices_data(0),
                      index_size(no_indices),indices_count(0),indices_data(0) {}
public:
    size_t read_header(const void *data,size_t size,int version); //0 if invalid

public:
    size_t get_chunk_size() const { return write_to_buf(0,0); }
    size_t write_to_buf(void *to_data,size_t to_size) const;
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
        nya_math::vec4 value;
    };

    struct int_param
    {
        std::string name;
        int value;

        int_param(): value(0) {}
    };

    struct material_info
    {
        std::string name;
        std::vector<texture_info> textures;
        std::vector<string_param> strings;
        std::vector<vector_param> vectors;
        std::vector<int_param> ints;

    public:
        void add_texture_info(const char *semantics,const char *filename,bool unique=true);
        void add_string_param(const char *name,const char *value,bool unique=true);
        void add_vector_param(const char *name,const nya_math::vec4 &value,bool unique=true);
        void add_int_param(const char *name,int value,bool unique=true);
    };

    std::vector<material_info> materials;

public:
    bool read(const void *data,size_t size,int version);

public:
    size_t get_chunk_size() const { return write_to_buf(0,0); }
    size_t write_to_buf(void *to_data,size_t to_size) const;
};

struct nms_skeleton_chunk
{
    struct bone
    {
        std::string name;
        nya_math::quat rot;
        nya_math::vec3 pos;
        int parent;

        bone(): parent(-1) {}
    };

    std::vector<bone> bones;

public:
    void sort();
    int get_bone_idx(const char *name) const;
    bone &add_bone(const char *name);

public:
    bool read(const void *data,size_t size,int version);

public:
    size_t get_chunk_size() const { return write_to_buf(0,0); }
    size_t write_to_buf(void *to_data,size_t to_size) const;
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
        nya_math::vec4 value;
    };

    struct object
    {
        std::string name;
        std::string type;
        std::vector<string_param> strings;
        std::vector<vector_param> vectors;

        void add_string_param(const char *name,const char *value,bool unique=true);
        void add_vector_param(const char *name,const nya_math::vec4 &value,bool unique=true);
    };

    std::vector<object> objects;

public:
    bool read(const void *data,size_t size,int version);

public:
    size_t get_chunk_size() const { return write_to_buf(0,0); }
    size_t write_to_buf(void *to_data,size_t to_size) const;
};

}
