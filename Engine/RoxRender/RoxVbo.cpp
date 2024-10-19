//nya-engine (C) nyan.developer@gmail.com released under the MIT license (see LICENSE)

#include "RoxVbo.h"
#include "RoxRenderApi.h"
#include "RoxStatistics.h"
#include "RoxMemory/tmp_buffer.h"

typedef unsigned int uint;

namespace RoxRender
{

namespace
{
    uint active_vert_count=0;
    uint active_ind_count=0;
    RoxVbo::ElementType active_ElementType=RoxVbo::TRIANGLES;
}

void RoxVbo::bindVerts() const
{
    getApiState().vertex_buffer= m_verts;
    active_ElementType= m_ElementType;
    active_vert_count= m_vert_count;
}

void RoxVbo::bindIndices() const
{
    getApiState().index_buffer=m_indices;
    active_ind_count=m_ind_count;
}

void RoxVbo::unbind() { getApiState().vertex_buffer=getApiState().index_buffer= -1; }

void RoxVbo::draw() { draw(0,getApiState().index_buffer<0?active_vert_count:active_ind_count,active_ElementType); }
void RoxVbo::draw(uint count) { draw(0,count,active_ElementType); }

void RoxVbo::draw(uint offset,uint count, ElementType el_type,uint instances)
{
    RoxRenderApiInterface::State &s=getApiState();

    if(offset+count>(s.index_buffer<0?active_vert_count:active_ind_count))
        return;

    s.primitive=el_type;
    s.index_offset=offset;
    s.index_count=count;
    s.instances_count=instances;

    getApiInterface().draw(s);

    if(Statistics::enabled())
    {
        ++Statistics::get().draw_count;
        Statistics::get().verts_count+=count*instances;

        const uint tri_count=(el_type==RoxVbo::TRIANGLES?count/3:(el_type==RoxVbo::TRIANGLE_STRIP?count-2:0))*instances;
        if(get_state().blend)
            Statistics::get().transparent_poly_count+=tri_count;
        else
            Statistics::get().opaque_poly_count+=tri_count;
    }
}

void RoxVbo::transformFeedback(RoxVbo &target)
{
    RoxRenderApiInterface::TfState s;
    (RoxRenderApiInterface::RenderState&)s=(RoxRenderApiInterface::RenderState&)getApiState();
    s.vertex_buffer_out=target.m_verts;
    s.primitive=active_ElementType;
    s.index_offset=s.out_offset=0;
    s.index_count= getApiState().index_buffer<0?active_vert_count:active_ind_count;
    s.instances_count=1;
    getApiInterface().transformFeedback(s);
}

void RoxVbo::transformFeedback(RoxVbo &target,uint src_offset,uint dst_offset,uint count,ElementType type)
{
    RoxRenderApiInterface::TfState s;

    if(src_offset+count>(s.index_buffer<0?active_vert_count:active_ind_count))
        return;

    (RoxRenderApiInterface::RenderState &)s=(RoxRenderApiInterface::RenderState&)getApiState();
    s.vertex_buffer_out=target.m_verts;
    s.primitive=type;
    s.index_offset=src_offset;
    s.out_offset=dst_offset;
    s.index_count=count;
    s.instances_count=1;
    getApiInterface().transformFeedback(s);
}

bool RoxVbo::set_vertex_data(const void*data,uint vert_stride,uint vert_count,usage_hint usage)
{
    RoxRenderApiInterface &api=getApiInterface();

    if(m_verts>=0)
    {
        if(vert_stride==m_stride && vert_count==m_vert_count)
        {
            api.updateVertexBuffer(m_verts,data);
            return true;
        }

        elease_verts();
    }

    if(!vert_count || !vert_stride)
        return false;

    m_verts=api.create_vertex_buffer(data,vert_stride,vert_count);
    if(m_verts<0)
        return false;

    m_vert_count=vert_count;
    m_stride=vert_stride;
    getApiInterface().set_vertex_layout(m_verts,m_layout);
    return true;
}

bool RoxVbo::set_index_data(const void*data,index_size size,uint indices_count,usage_hint usage)
{
    RoxRenderApiInterface &api=getApiInterface();

    if(m_indices>=0)
        release_indices();

    m_indices=api.create_index_buffer(data,size,indices_count,usage);
    if(m_indices<0)
        return false;

    m_ind_count=indices_count;
    m_ind_size=size;
    return true;
}

void RoxVbo::set_vertices(uint offset,uint dimension,vertex_atrib_type type)
{
    if(dimension>4)
        return;

    m_layout.pos.offset=offset;
    m_layout.pos.dimension=dimension;
    m_layout.pos.type=type;
    set_layout(m_layout);
}

void RoxVbo::set_normals(uint offset,vertex_atrib_type type)
{
    m_layout.normal.offset=offset;
    m_layout.normal.dimension=3;
    m_layout.normal.type=type;
    set_layout(m_layout);
}

void RoxVbo::set_tc(uint tc_idx,uint offset,uint dimension,vertex_atrib_type type)
{
    if(tc_idx>=max_tex_coord || dimension>4)
        return;

    m_layout.tc[tc_idx].offset=offset;
    m_layout.tc[tc_idx].dimension=dimension;
    m_layout.tc[tc_idx].type=type;
    set_layout(m_layout);
}

void RoxVbo::set_colors(uint offset,uint dimension,vertex_atrib_type type)
{
    if(dimension>4)
        return;

    m_layout.color.offset=offset;
    m_layout.color.dimension=dimension;
    m_layout.color.type=type;
    set_layout(m_layout);
}

void RoxVbo::set_layout(const layout &l)
{
    m_layout=l;
    if(m_verts>=0)
        getApiInterface().set_vertex_layout(m_verts,m_layout);
}

void RoxVbo::set_ElementType(ElementType type)
{
    if(m_verts>=0 && getApiState().vertex_buffer==m_verts)
        active_ElementType=type;
    m_ElementType=type;
}

bool RoxVbo::get_vertex_data(nya_memory::tmp_buffer_ref &data) const
{
    if(m_verts<0)
    {
        data.free();
        return false;
    }

    data.allocate(m_stride*m_vert_count);
    if(!getApiInterface().get_vertex_data(m_verts,data.get_data()))
    {
        data.free();
        return false;
    }

    return true;
}

bool RoxVbo::get_index_data(nya_memory::tmp_buffer_ref &data) const
{
    if(m_indices<0)
    {
        data.free();
        return false;
    }

    data.allocate(m_ind_count*m_ind_size);
    if(!getApiInterface().get_index_data(m_indices,data.get_data()))
    {
        data.free();
        return false;
    }

    return true;
}

const RoxVbo::layout &RoxVbo::get_layout() const { return m_layout; }
RoxVbo::index_size RoxVbo::get_index_size() const { return m_ind_size; }
RoxVbo::ElementType RoxVbo::get_ElementType() const { return m_ElementType; }
uint RoxVbo::get_vert_stride() const{ return m_stride; }
uint RoxVbo::get_vert_offset() const { return m_layout.pos.offset; }
uint RoxVbo::get_vert_dimension() const { return m_layout.pos.dimension; }
uint RoxVbo::get_normals_offset() const { return m_layout.normal.offset; }
uint RoxVbo::get_tc_offset(uint idx) const { return idx<max_tex_coord?m_layout.tc[idx].offset:0; }
uint RoxVbo::get_tc_dimension(uint idx) const { return idx<max_tex_coord?m_layout.tc[idx].dimension:0; }
uint RoxVbo::get_colors_offset() const { return m_layout.color.offset; }
uint RoxVbo::get_colors_dimension() const { return m_layout.color.dimension; }
uint RoxVbo::get_verts_count() const { return m_vert_count; }
uint RoxVbo::get_indices_count() const { return m_ind_count; }

uint RoxVbo::get_used_vmem_size()
{
     //ToDo

    return 0;
}

void RoxVbo::release()
{
    release_verts();
    release_indices();
    m_layout=layout();
}

void RoxVbo::release_verts()
{
    if(m_verts<0)
        return;

    RoxRenderApiInterface::state &s=getApiState();
    getApiInterface().remove_vertex_buffer(m_verts);
    if(s.vertex_buffer==m_verts)
        s.vertex_buffer=-1;
    m_verts=-1;
    m_vert_count=0;
    m_stride=0;
}

void RoxVbo::release_indices()
{
    if(m_indices<0)
        return;

    RoxRenderApiInterface::state &s=getApiState();
    getApiInterface().remove_index_buffer(m_indices);
    if(s.index_buffer==m_indices)
        s.index_buffer=-1;
    m_indices=-1;
    m_ind_size=index2b;
    m_ind_count=0;
}

bool RoxVbo::is_transformFeedback_supported() { return getApiInterface().is_transformFeedback_supported(); }
}
