//nya-engine (C) nyan.developer@gmail.com released under the MIT license (see LICENSE)

#include "vbo.h"
#include "render_api.h"
#include "statistics.h"
#include "memory/tmp_buffer.h"

typedef unsigned int uint;

namespace nya_render
{

namespace
{
    uint active_vert_count=0;
    uint active_ind_count=0;
    vbo::element_type active_element_type=vbo::triangles;
}

void vbo::bind_verts() const
{
    get_api_state().vertex_buffer=m_verts;
    active_element_type=m_element_type;
    active_vert_count=m_vert_count;
}

void vbo::bind_indices() const
{
    get_api_state().index_buffer=m_indices;
    active_ind_count=m_ind_count;
}

void vbo::unbind() { get_api_state().vertex_buffer=get_api_state().index_buffer= -1; }

void vbo::draw() { draw(0,get_api_state().index_buffer<0?active_vert_count:active_ind_count,active_element_type); }
void vbo::draw(uint count) { draw(0,count,active_element_type); }

void vbo::draw(uint offset,uint count,element_type el_type,uint instances)
{
    render_api_interface::state &s=get_api_state();

    if(offset+count>(s.index_buffer<0?active_vert_count:active_ind_count))
        return;

    s.primitive=el_type;
    s.index_offset=offset;
    s.index_count=count;
    s.instances_count=instances;

    get_api_interface().draw(s);

    if(statistics::enabled())
    {
        ++statistics::get().draw_count;
        statistics::get().verts_count+=count*instances;

        const uint tri_count=(el_type==vbo::triangles?count/3:(el_type==vbo::triangle_strip?count-2:0))*instances;
        if(get_state().blend)
            statistics::get().transparent_poly_count+=tri_count;
        else
            statistics::get().opaque_poly_count+=tri_count;
    }
}

void vbo::transform_feedback(vbo &target)
{
    render_api_interface::tf_state s;
    (render_api_interface::render_state&)s=(render_api_interface::render_state&)get_api_state();
    s.vertex_buffer_out=target.m_verts;
    s.primitive=active_element_type;
    s.index_offset=s.out_offset=0;
    s.index_count=get_api_state().index_buffer<0?active_vert_count:active_ind_count;
    s.instances_count=1;
    get_api_interface().transform_feedback(s);
}

void vbo::transform_feedback(vbo &target,uint src_offset,uint dst_offset,uint count,element_type type)
{
    render_api_interface::tf_state s;

    if(src_offset+count>(s.index_buffer<0?active_vert_count:active_ind_count))
        return;

    (render_api_interface::render_state&)s=(render_api_interface::render_state&)get_api_state();
    s.vertex_buffer_out=target.m_verts;
    s.primitive=type;
    s.index_offset=src_offset;
    s.out_offset=dst_offset;
    s.index_count=count;
    s.instances_count=1;
    get_api_interface().transform_feedback(s);
}

bool vbo::set_vertex_data(const void*data,uint vert_stride,uint vert_count,usage_hint usage)
{
    render_api_interface &api=get_api_interface();

    if(m_verts>=0)
    {
        if(vert_stride==m_stride && vert_count==m_vert_count)
        {
            api.update_vertex_buffer(m_verts,data);
            return true;
        }

        release_verts();
    }

    if(!vert_count || !vert_stride)
        return false;

    m_verts=api.create_vertex_buffer(data,vert_stride,vert_count);
    if(m_verts<0)
        return false;

    m_vert_count=vert_count;
    m_stride=vert_stride;
    get_api_interface().set_vertex_layout(m_verts,m_layout);
    return true;
}

bool vbo::set_index_data(const void*data,index_size size,uint indices_count,usage_hint usage)
{
    render_api_interface &api=get_api_interface();

    if(m_indices>=0)
        release_indices();

    m_indices=api.create_index_buffer(data,size,indices_count,usage);
    if(m_indices<0)
        return false;

    m_ind_count=indices_count;
    m_ind_size=size;
    return true;
}

void vbo::set_vertices(uint offset,uint dimension,vertex_atrib_type type)
{
    if(dimension>4)
        return;

    m_layout.pos.offset=offset;
    m_layout.pos.dimension=dimension;
    m_layout.pos.type=type;
    set_layout(m_layout);
}

void vbo::set_normals(uint offset,vertex_atrib_type type)
{
    m_layout.normal.offset=offset;
    m_layout.normal.dimension=3;
    m_layout.normal.type=type;
    set_layout(m_layout);
}

void vbo::set_tc(uint tc_idx,uint offset,uint dimension,vertex_atrib_type type)
{
    if(tc_idx>=max_tex_coord || dimension>4)
        return;

    m_layout.tc[tc_idx].offset=offset;
    m_layout.tc[tc_idx].dimension=dimension;
    m_layout.tc[tc_idx].type=type;
    set_layout(m_layout);
}

void vbo::set_colors(uint offset,uint dimension,vertex_atrib_type type)
{
    if(dimension>4)
        return;

    m_layout.color.offset=offset;
    m_layout.color.dimension=dimension;
    m_layout.color.type=type;
    set_layout(m_layout);
}

void vbo::set_layout(const layout &l)
{
    m_layout=l;
    if(m_verts>=0)
        get_api_interface().set_vertex_layout(m_verts,m_layout);
}

void vbo::set_element_type(element_type type)
{
    if(m_verts>=0 && get_api_state().vertex_buffer==m_verts)
        active_element_type=type;
    m_element_type=type;
}

bool vbo::get_vertex_data(nya_memory::tmp_buffer_ref &data) const
{
    if(m_verts<0)
    {
        data.free();
        return false;
    }

    data.allocate(m_stride*m_vert_count);
    if(!get_api_interface().get_vertex_data(m_verts,data.get_data()))
    {
        data.free();
        return false;
    }

    return true;
}

bool vbo::get_index_data(nya_memory::tmp_buffer_ref &data) const
{
    if(m_indices<0)
    {
        data.free();
        return false;
    }

    data.allocate(m_ind_count*m_ind_size);
    if(!get_api_interface().get_index_data(m_indices,data.get_data()))
    {
        data.free();
        return false;
    }

    return true;
}

const vbo::layout &vbo::get_layout() const { return m_layout; }
vbo::index_size vbo::get_index_size() const { return m_ind_size; }
vbo::element_type vbo::get_element_type() const { return m_element_type; }
uint vbo::get_vert_stride() const{ return m_stride; }
uint vbo::get_vert_offset() const { return m_layout.pos.offset; }
uint vbo::get_vert_dimension() const { return m_layout.pos.dimension; }
uint vbo::get_normals_offset() const { return m_layout.normal.offset; }
uint vbo::get_tc_offset(uint idx) const { return idx<max_tex_coord?m_layout.tc[idx].offset:0; }
uint vbo::get_tc_dimension(uint idx) const { return idx<max_tex_coord?m_layout.tc[idx].dimension:0; }
uint vbo::get_colors_offset() const { return m_layout.color.offset; }
uint vbo::get_colors_dimension() const { return m_layout.color.dimension; }
uint vbo::get_verts_count() const { return m_vert_count; }
uint vbo::get_indices_count() const { return m_ind_count; }

uint vbo::get_used_vmem_size()
{
     //ToDo

    return 0;
}

void vbo::release()
{
    release_verts();
    release_indices();
    m_layout=layout();
}

void vbo::release_verts()
{
    if(m_verts<0)
        return;

    render_api_interface::state &s=get_api_state();
    get_api_interface().remove_vertex_buffer(m_verts);
    if(s.vertex_buffer==m_verts)
        s.vertex_buffer=-1;
    m_verts=-1;
    m_vert_count=0;
    m_stride=0;
}

void vbo::release_indices()
{
    if(m_indices<0)
        return;

    render_api_interface::state &s=get_api_state();
    get_api_interface().remove_index_buffer(m_indices);
    if(s.index_buffer==m_indices)
        s.index_buffer=-1;
    m_indices=-1;
    m_ind_size=index2b;
    m_ind_count=0;
}

bool vbo::is_transform_feedback_supported() { return get_api_interface().is_transform_feedback_supported(); }
}
