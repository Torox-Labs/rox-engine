//nya-engine (C) nyan.developer@gmail.com released under the MIT license (see LICENSE)

#pragma once

namespace nya_memory { class tmp_buffer_ref; }

namespace nya_render
{

class vbo
{
public:
    enum element_type
    {
        triangles,
        triangle_strip,
        points,
        lines,
        line_strip
    };

    enum index_size
    {
        index2b=2,
        index4b=4
    };

    enum vertex_atrib_type
    {
        float16,
        float32,
        uint8
    };

    enum usage_hint
    {
        static_draw,
        dynamic_draw,
        stream_draw
    };

    typedef unsigned int uint;

    const static uint max_tex_coord=13;

    struct layout
    {
        struct attribute { unsigned char offset,dimension; vertex_atrib_type type; attribute():offset(0),dimension(0),type(float32){} };
        attribute pos,normal,color;
        attribute tc[max_tex_coord];

        layout() { pos.dimension=3; }
    };

public:
    bool set_vertex_data(const void*data,uint vert_stride,uint vert_count,usage_hint usage=static_draw);
    bool set_index_data(const void*data,index_size size,uint indices_count,usage_hint usage=static_draw);
    void set_element_type(element_type type);
    void set_vertices(uint offset,uint dimension,vertex_atrib_type=float32);
    void set_normals(uint offset,vertex_atrib_type=float32);
    void set_tc(uint tc_idx,uint offset,uint dimension,vertex_atrib_type=float32);
    void set_colors(uint offset,uint dimension,vertex_atrib_type=float32);
    void set_layout(const layout &l);

public:
    bool get_vertex_data(nya_memory::tmp_buffer_ref &data) const;
    bool get_index_data(nya_memory::tmp_buffer_ref &data) const;
    element_type get_element_type() const;
    uint get_verts_count() const;
    uint get_vert_stride() const;
    uint get_vert_offset() const;
    uint get_vert_dimension() const;
    uint get_normals_offset() const;
    uint get_tc_offset(uint idx) const;
    uint get_tc_dimension(uint idx) const;
    uint get_colors_offset() const;
    uint get_colors_dimension() const;
    uint get_indices_count() const;
    index_size get_index_size() const;
    const layout &get_layout() const;

public:
    void bind() const { bind_verts(); bind_indices(); }

    void bind_verts() const;
    void bind_indices() const;

    static void unbind();

public:
    static void draw();
    static void draw(uint count);
    static void draw(uint offset,uint count,element_type type=triangles,uint instances=1);
    static void transform_feedback(vbo &target);
    static void transform_feedback(vbo &target,uint src_offset,uint dst_offset,uint count,element_type type=points);

public:
    void release();

private:
    void release_verts();
    void release_indices();

public:
    static uint get_used_vmem_size();
    
public:
    static bool is_transform_feedback_supported();

public:
    vbo(): m_verts(-1),m_indices(-1),m_vert_count(0),m_ind_count(0),
           m_stride(0),m_ind_size(index2b),m_element_type(triangles) {}

private:
    int m_verts;
    int m_indices;
    int m_vert_count;
    int m_ind_count;
    layout m_layout;
    int m_stride;
    index_size m_ind_size;
    element_type m_element_type;
};

}
