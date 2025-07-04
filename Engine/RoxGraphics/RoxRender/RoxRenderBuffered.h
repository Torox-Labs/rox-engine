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

#include "IRoxRenderAPI.h"

#include <queue>
#include <cstring>

namespace RoxRender
{
	class RoxRenderBuffered : public IRoxRenderAPI
	{
	public:
		int createShader(const char* vertex, const char* fragment) override;
		uint getUniformsCount(int shader) override;
		RoxShader::Uniform getUniform(int shader, int idx) override;
		void removeShader(int shader) override;

		int createUniformBuffer(int shader) override;
		void setUniform(int uniform_buffer, int idx, const float* buf, uint count) override;
		void removeUniformBuffer(int uniform_buffer) override;

	public:
		int createVertexBuffer(const void* data, uint stride, uint count, RoxVBO::USAGE_HINT usage) override;
		void setVertexLayout(int idx, RoxVBO::Layout layout) override;
		void updateVertexBuffer(int idx, const void* data) override;
		bool getVertexData(int idx, void* data) override;
		void removeVertexBuffer(int idx) override;

		int createIndexBuffer(const void* data, RoxVBO::INDEX_SIZE type, uint count, RoxVBO::USAGE_HINT usage) override;
		void updateIndexBuffer(int idx, const void* data) override;
		bool getIndexData(int idx, void* data) override;
		void removeIndexBuffer(int idx) override;

	public:
		int createTexture(const void* data, uint width, uint height, RoxTexture::COLOR_FORMAT& format,
		                  int mip_count) override;
		int createCubemap(const void* data[6], uint width, RoxTexture::COLOR_FORMAT& format, int mip_count) override;
		void updateTexture(int idx, const void* data, uint x, uint y, uint width, uint height, int mip) override;
		void setTextureWrap(int idx, RoxTexture::WRAP s, RoxTexture::WRAP t) override;
		void setTextureFilter(int idx, RoxTexture::FILTER min, RoxTexture::FILTER mag, RoxTexture::FILTER mip,
		                      uint aniso) override;
		bool getTextureData(int RoxTexture, uint x, uint y, uint w, uint h, void* data) override;

		void removeTexture(int RoxTexture) override;
		uint getMaxTextureDimension() override { return m_max_texture_dimention; }
		bool isTextureFormatSupported(RoxTexture::COLOR_FORMAT format) override;

	public:
		int createTarget(uint width, uint height, uint samples, const int* attachment_textures,
		                 const int* attachment_sides, uint attachment_count, int depth_texture) override;
		void resolveTarget(int idx) override;
		void removeTarget(int idx) override;
		uint getMaxTargetAttachments() override { return m_max_target_attachments; }
		uint getMaxTargetMsaa() override { return m_max_target_msaa; }

	public:
		void setCamera(const RoxMath::Matrix4& modelview, const RoxMath::Matrix4& projection) override;
		void clear(const ViewportState& s, bool color, bool depth, bool stencil) override;
		void draw(const State& s) override;

		void invalidateCachedState() override;
		void applyState(const State& s) override;

	public:
		size_t getBufferSize() const { return m_current.buffer.size() * sizeof(int); }

		void commit(); //curr -> pending
		void push(); //pending -> processing
		void execute(); //run processing

	public:
		RoxRenderBuffered(IRoxRenderAPI& backend) : m_backend(backend)
		{
			m_max_texture_dimention = m_backend.getMaxTextureDimension();
			m_max_target_attachments = m_backend.getMaxTargetAttachments();
			m_max_target_msaa = m_backend.getMaxTargetMsaa();
			for (int i = 0; i < int(sizeof(m_tex_formats) / sizeof(m_tex_formats[0])); ++i)
				m_tex_formats[i] = m_backend.isTextureFormatSupported(RoxTexture::COLOR_FORMAT(i));
		}

	private:
		int newIdx();
		void remapIdx(int& idx) const;
		void remapState(State& s) const;

	private:
		IRoxRenderAPI& m_backend;

		uint m_max_texture_dimention, m_max_target_attachments, m_max_target_msaa;
		bool m_tex_formats[64];
		std::map<int, std::vector<RoxShader::Uniform>> m_uniform_info;
		std::map<int, uint> m_buf_sizes;

		enum COMMAND_TYPE
		{
			CMD_CLEAR = 0x637200,
			CMD_CAMERA,
			CMD_APPLY,
			CMD_DRAW,
			CMD_UNIFORM,
			CMD_RESOLVE,

			CMD_SHDR_CREATE,
			CMD_SHDR_REMOVE,
			CMD_UBUF_CREATE,
			CMD_UBUF_REMOVE,

			CMD_VBUF_CREATE,
			CMD_VBUF_LAYOUT,
			CMD_VBUF_UPDATE,
			CMD_VBUF_REMOVE,

			CMD_IBUF_CREATE,
			CMD_IBUF_UPDATE,
			CMD_IBUF_REMOVE,

			CMD_TEX_CREATE,
			CMD_TEX_CUBE,
			CMD_TEX_UPDATE,
			CMD_TEX_WRAP,
			CMD_TEX_FILTER,
			CMD_TEX_REMOVE,

			CMD_TARGET_CREATE,
			CMD_TARGET_REMOVE,

			CMD_INVALIDATE
		};

		struct CommandBuffer
		{
			void write(int command) { buffer.push_back(command); }

			template <typename t>
			void write(COMMAND_TYPE command, const t& data)
			{
				const size_t offset = buffer.size();
				buffer.resize(offset + (sizeof(t) + 3) / sizeof(int) + 1);
				buffer[offset] = command;
				memcpy(buffer.data() + (offset + 1), &data, sizeof(data));
			}

			template <typename t>
			void write(COMMAND_TYPE command, const t& data, int count, const float* buf)
			{
				const size_t offset = buffer.size();
				buffer.resize(offset + (sizeof(t) + 3) / sizeof(int) + 1 + count);
				buffer[offset] = command;
				memcpy(buffer.data() + (offset + 1), &data, sizeof(data));
				memcpy(buffer.data() + (buffer.size() - count), buf, count * sizeof(float));
			}

			void write(int byte_count, const void* buf)
			{
				const size_t offset = buffer.size();
				buffer.resize(offset + (byte_count + 3) / sizeof(int));
				memcpy(buffer.data() + offset, buf, byte_count);
			}

			COMMAND_TYPE getCmd() { return COMMAND_TYPE(buffer[(++read_offset) - 1]); }

			template <typename t>
			t& getCmdData()
			{
				const size_t size = (sizeof(t) + 3) / sizeof(int);
				return *((t*)&buffer[(read_offset += size) - size]);
			}

			float* getFbuf(int count) { return (float*)&buffer[(read_offset += count) - count]; }

			void* getCbuf(int size)
			{
				const size_t s = (size + 3) / sizeof(int);
				return &buffer[(read_offset += s) - s];
			}

			std::vector<int> buffer;
			size_t read_offset;

			std::vector<int> remap;
			std::vector<int> remap_free;
			bool update_remap;

			CommandBuffer() : read_offset(0), update_remap(false)
			{
			}
		};

		CommandBuffer m_current;
		CommandBuffer m_pending;
		CommandBuffer m_processing;

		struct uniform_data
		{
			int buf_idx, idx;
			uint count;
		};

		struct clear_data
		{
			ViewportState vp;
			bool color, depth, stencil, reserved;
		};

		struct camera_data
		{
			RoxMath::Matrix4 mv, p;
		};

		struct shader_create_data
		{
			int idx, vs_size, ps_size;
		};

		struct ubuf_create_data
		{
			int idx, shader_idx;
		};

		struct vbuf_create_data
		{
			int idx;
			uint stride, count;
			RoxVBO::USAGE_HINT usage;
		};

		struct vbuf_layout
		{
			int idx;
			RoxVBO::Layout layout;
		};

		struct buf_update
		{
			int idx;
			uint size;
		};

		struct ibuf_create_data
		{
			int idx;
			RoxVBO::INDEX_SIZE type;
			uint count;
			RoxVBO::USAGE_HINT usage;
		};

		struct tex_create_data
		{
			int idx;
			uint width, height, size;
			RoxTexture::COLOR_FORMAT format;
			int mip_count;
		};

		struct tex_update
		{
			int idx;
			uint x, y, width, height, size;
			int mip;
		};

		struct tex_wrap
		{
			int idx;
			RoxTexture::WRAP s, t;
		};

		struct tex_filter
		{
			int idx;
			RoxTexture::FILTER minification, magnification, mipmap;
			uint aniso;
		};

		struct target_create
		{
			int idx;
			uint w, h, s, count;
			int at[16];
			int as[16];
			int d;
		};
	};
}
