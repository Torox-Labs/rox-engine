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

#include "RoxVBO.h"
#include "IRoxRenderAPI.h"
#include "RoxStatistics.h"
#include "RoxMemory/RoxTmpBuffers.h"

using uint = unsigned int;

namespace RoxRender
{
	namespace
	{
		uint active_vert_count = 0;
		uint active_ind_count = 0;
		RoxVBO::ELEMENT_TYPE active_ElementType = RoxVBO::TRIANGLES;
	}

	void RoxVBO::bindVerts() const
	{
		getAPIState().vertex_buffer = m_verts;
		active_ElementType = m_ElementType;
		active_vert_count = m_vert_count;
	}

	void RoxVBO::bindIndices() const
	{
		getAPIState().index_buffer = m_indices;
		active_ind_count = m_ind_count;
	}

	void RoxVBO::unbind() { getAPIState().vertex_buffer = getAPIState().index_buffer = -1; }

	void RoxVBO::draw()
	{
		draw(0, getAPIState().index_buffer < 0 ? active_vert_count : active_ind_count, active_ElementType);
	}

	void RoxVBO::draw(uint count) { draw(0, count, active_ElementType); }

	void RoxVBO::draw(uint offset, uint count, ELEMENT_TYPE el_type, uint instances)
	{
		IRoxRenderAPI::State& s = getAPIState();

		if (offset + count > (s.index_buffer < 0 ? active_vert_count : active_ind_count))
			return;

		s.primitive = el_type;
		s.index_offset = offset;
		s.index_count = count;
		s.instances_count = instances;

		getAPIInterface().draw(s);

		if (Statistics::enabled())
		{
			++Statistics::get().draw_count;
			Statistics::get().verts_count += count * instances;

			const uint tri_count = (el_type == RoxVBO::TRIANGLES
				                        ? count / 3
				                        : (el_type == RoxVBO::TRIANGLE_STRIP ? count - 2 : 0)) * instances;
			if (getState().blend)
				Statistics::get().transparent_poly_count += tri_count;
			else
				Statistics::get().opaque_poly_count += tri_count;
		}
	}

	void RoxVBO::transformFeedback(RoxVBO& target)
	{
		IRoxRenderAPI::TfState s;
		(IRoxRenderAPI::RenderState&)s = (IRoxRenderAPI::RenderState&)getAPIState();
		s.vertex_buffer_out = target.m_verts;
		s.primitive = active_ElementType;
		s.index_offset = s.out_offset = 0;
		s.index_count = getAPIState().index_buffer < 0 ? active_vert_count : active_ind_count;
		s.instances_count = 1;
		getAPIInterface().transformFeedback(s);
	}

	void RoxVBO::transformFeedback(RoxVBO& target, uint src_offset, uint dst_offset, uint count, ELEMENT_TYPE type)
	{
		IRoxRenderAPI::TfState s;

		if (src_offset + count > (s.index_buffer < 0 ? active_vert_count : active_ind_count))
			return;

		(IRoxRenderAPI::RenderState&)s = (IRoxRenderAPI::RenderState&)getAPIState();
		s.vertex_buffer_out = target.m_verts;
		s.primitive = type;
		s.index_offset = src_offset;
		s.out_offset = dst_offset;
		s.index_count = count;
		s.instances_count = 1;
		getAPIInterface().transformFeedback(s);
	}

	bool RoxVBO::setVertexData(const void* data, uint vert_stride, uint vert_count, USAGE_HINT usage)
	{
		IRoxRenderAPI& api = getAPIInterface();

		if (m_verts >= 0)
		{
			if (vert_stride == m_stride && vert_count == m_vert_count)
			{
				api.updateVertexBuffer(m_verts, data);
				return true;
			}

			releaseVerts();
		}

		if (!vert_count || !vert_stride)
			return false;

		m_verts = api.createVertexBuffer(data, vert_stride, vert_count);
		if (m_verts < 0)
			return false;

		m_vert_count = vert_count;
		m_stride = vert_stride;
		getAPIInterface().setVertexLayout(m_verts, m_layout);
		return true;
	}

	bool RoxVBO::setIndexData(const void* data, INDEX_SIZE size, uint indices_count, USAGE_HINT usage)
	{
		IRoxRenderAPI& api = getAPIInterface();

		if (m_indices >= 0)
			releaseIndices();

		m_indices = api.createIndexBuffer(data, size, indices_count, usage);
		if (m_indices < 0)
			return false;

		m_ind_count = indices_count;
		m_ind_size = size;
		return true;
	}

	void RoxVBO::setVertices(uint offset, uint dimension, VERTEX_ATRIB_TYPE type)
	{
		if (dimension > 4)
			return;

		m_layout.pos.offset = offset;
		m_layout.pos.dimension = dimension;
		m_layout.pos.type = type;
		setLayout(m_layout);
	}

	void RoxVBO::setNormals(uint offset, VERTEX_ATRIB_TYPE type)
	{
		m_layout.normal.offset = offset;
		m_layout.normal.dimension = 3;
		m_layout.normal.type = type;
		setLayout(m_layout);
	}

	void RoxVBO::setTc(uint tc_idx, uint offset, uint dimension, VERTEX_ATRIB_TYPE type)
	{
		if (tc_idx >= max_tex_coord || dimension > 4)
			return;

		m_layout.tc[tc_idx].offset = offset;
		m_layout.tc[tc_idx].dimension = dimension;
		m_layout.tc[tc_idx].type = type;
		setLayout(m_layout);
	}

	void RoxVBO::setColors(uint offset, uint dimension, VERTEX_ATRIB_TYPE type)
	{
		if (dimension > 4)
			return;

		m_layout.color.offset = offset;
		m_layout.color.dimension = dimension;
		m_layout.color.type = type;
		setLayout(m_layout);
	}

	void RoxVBO::setLayout(const Layout& l)
	{
		m_layout = l;
		if (m_verts >= 0)
			getAPIInterface().setVertexLayout(m_verts, m_layout);
	}

	void RoxVBO::setElementType(ELEMENT_TYPE type)
	{
		if (m_verts >= 0 && getAPIState().vertex_buffer == m_verts)
			active_ElementType = type;
		m_ElementType = type;
	}

	bool RoxVBO::getVertexData(RoxMemory::RoxTmpBufferRef& data) const
	{
		if (m_verts < 0)
		{
			data.free();
			return false;
		}

		data.allocate(m_stride * m_vert_count);
		if (!getAPIInterface().getVertexData(m_verts, data.getData()))
		{
			data.free();
			return false;
		}

		return true;
	}

	bool RoxVBO::getIndexData(RoxMemory::RoxTmpBufferRef& data) const
	{
		if (m_indices < 0)
		{
			data.free();
			return false;
		}

		data.allocate(m_ind_count * m_ind_size);
		if (!getAPIInterface().getIndexData(m_indices, data.getData()))
		{
			data.free();
			return false;
		}

		return true;
	}

	const RoxVBO::Layout& RoxVBO::getLayout() const { return m_layout; }
	RoxVBO::INDEX_SIZE RoxVBO::getIndexSize() const { return m_ind_size; }
	RoxVBO::ELEMENT_TYPE RoxVBO::getElementType() const { return m_ElementType; }
	uint RoxVBO::getVertStride() const { return m_stride; }
	uint RoxVBO::getVertOffset() const { return m_layout.pos.offset; }
	uint RoxVBO::getVertDimension() const { return m_layout.pos.dimension; }
	uint RoxVBO::getNormalsOffset() const { return m_layout.normal.offset; }
	uint RoxVBO::getTcOffset(uint idx) const { return idx < max_tex_coord ? m_layout.tc[idx].offset : 0; }
	uint RoxVBO::getTcDimension(uint idx) const { return idx < max_tex_coord ? m_layout.tc[idx].dimension : 0; }
	uint RoxVBO::getColorsOffset() const { return m_layout.color.offset; }
	uint RoxVBO::getColorsDimension() const { return m_layout.color.dimension; }
	uint RoxVBO::getVertsCount() const { return m_vert_count; }
	uint RoxVBO::getIndicesCount() const { return m_ind_count; }

	uint RoxVBO::getUsedVmemSize()
	{
		//ToDo

		return 0;
	}

	void RoxVBO::release()
	{
		releaseVerts();
		releaseIndices();
		m_layout = Layout();
	}

	void RoxVBO::releaseVerts()
	{
		if (m_verts < 0)
			return;

		IRoxRenderAPI::State& s = getAPIState();
		getAPIInterface().removeVertexBuffer(m_verts);
		if (s.vertex_buffer == m_verts)
			s.vertex_buffer = -1;
		m_verts = -1;
		m_vert_count = 0;
		m_stride = 0;
	}

	void RoxVBO::releaseIndices()
	{
		if (m_indices < 0)
			return;

		IRoxRenderAPI::State& s = getAPIState();
		getAPIInterface().removeIndexBuffer(m_indices);
		if (s.index_buffer == m_indices)
			s.index_buffer = -1;
		m_indices = -1;
		m_ind_size = INDEX_2D;
		m_ind_count = 0;
	}

	bool RoxVBO::isTransformFeedbackSupported() { return getAPIInterface().isTransformFeedbackSupported(); }
}
