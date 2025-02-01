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

#include "RoxRender.h"
#include "RoxShader.h"
#include "RoxTexture.h"
#include "RoxVbo.h"

namespace RoxRender
{
	class IRoxRenderApi
	{
	public:
		virtual bool isAvailable() const { return true; }

		typedef unsigned int uint;

		struct ViewportState
		{
			Rectangle viewport;
			Rectangle scissor;
			bool scissor_enabled;
			float clear_color[4];
			float clear_depth;
			uint clear_stencil;
			int target;

			ViewportState(): scissor_enabled(false), target(-1)
			{
				for (int i = 0; i < 4; ++i) clear_color[i] = 0.0f;
				clear_depth = 1.0f;
				clear_stencil = 0;
			}
		};

		struct RenderState
		{
			int vertex_buffer;
			int index_buffer;
			RoxVbo::ELEMENT_TYPE primitive;
			uint index_offset;
			uint index_count;
			uint instances_count;

			int shader;
			int uniform_buffer;

			static const uint max_layers = 8;
			int textures[max_layers];

			RenderState()
			{
				vertex_buffer = index_buffer = -1;
				primitive = RoxVbo::TRIANGLES;
				index_offset = index_count = instances_count = 0;
				shader = uniform_buffer = -1;
				for (int i = 0; i < max_layers; ++i)
					textures[i] = -1;
			}
		};

		struct State : public ViewportState, RenderState, RoxRender::State
		{
		};

		struct TfState : public RenderState
		{
			int vertex_buffer_out;
			uint out_offset;

			TfState(): vertex_buffer_out(-1), out_offset(0)
			{
			}
		};

	public:
		virtual int createShader(const char* vertex, const char* fragment) { return -1; }
		virtual uint getUniformsCount(int shader) { return 0; }
		virtual RoxShader::Uniform getUniform(int shader, int idx) { return RoxRender::RoxShader::Uniform(); }

		virtual void removeShader(int shader)
		{
		}

		virtual int createUniformBuffer(int shader) { return -1; }

		virtual void setUniform(int uniform_buffer, int idx, const float* buf, uint count)
		{
		}

		virtual void removeUniformBuffer(int uniform_buffer)
		{
		}

	public:
		virtual int createVertexBuffer(const void* data, uint stride, uint count,
		                                 RoxVbo::USAGE_HINT usage = RoxVbo::STATIC_DRAW) { return -1; }

		virtual void setVertexLayout(int idx, RoxVbo::Layout layout)
		{
		}

		virtual void updateVertexBuffer(int idx, const void* data)
		{
		}

		virtual bool getVertexData(int idx, void* data) { return false; }

		virtual void removeVertexBuffer(int idx)
		{
		}

		virtual int createIndexBuffer(const void* data, RoxVbo::INDEX_SIZE type, uint count,
		                                RoxVbo::USAGE_HINT usage = RoxVbo::STATIC_DRAW) { return -1; }

		virtual void updateIndexBuffer(int idx, const void* data)
		{
		}

		virtual bool getIndexData(int idx, void* data) { return false; }

		virtual void removeIndexBuffer(int idx)
		{
		}

	public:
		virtual int createTexture(const void* data, uint width, uint height, RoxRender::RoxTexture::COLOR_FORMAT & format,
		                           int mip_count) { return -1; }

		virtual int createCubemap(const void* data[6], uint width, RoxRender::RoxTexture::COLOR_FORMAT & format,
		                           int mip_count)
		{
			return -1;
		}

		virtual void updateTexture(int idx, const void* data, uint x, uint y, uint width, uint height, int mip)
		{
		}

		virtual void setTextureWrap(int idx, RoxRender::RoxTexture::WRAP s, RoxRender::RoxTexture::WRAP t)
		{
		}

		virtual void setTextureFilter(int idx, RoxRender::RoxTexture::FILTER minification,
		                                RoxRender::RoxTexture::FILTER magnification,
		                                RoxRender::RoxTexture::FILTER mipmap, uint aniso)
		{
		}

		virtual bool getTextureData(int texture, uint x, uint y, uint w, uint h, void* data) { return false; }

		virtual void removeTexture(int texture)
		{
		}

		virtual uint getMaxTextureDimension() { return 0; }
		virtual bool isTextureFormatSupported(RoxRender::RoxTexture::COLOR_FORMAT format) { return false; }

	public:
		virtual int createTarget(uint width, uint height, uint samples, const int* attachment_textures,
		                          const int* attachment_sides, uint attachment_count, int depth_texture) { return -1; }

		virtual void resolveTarget(int idx)
		{
		}

		virtual void removeTarget(int idx)
		{
		}

		virtual uint getMaxTargetAttachments() { return 0; }
		virtual uint getMaxTargetMsaa() { return 0; }

	public:
		virtual void setCamera(const RoxMath::Matrix4& model_view, const RoxMath::Matrix4& projection)
		{
		}

		virtual void clear(const ViewportState& s, bool color, bool depth, bool stencil)
		{
		}

		virtual void draw(const State& s)
		{
		}

		virtual void transformFeedback(const TfState& s)
		{
		}

		virtual bool isTransformFeedbackSupported() { return false; }

		virtual void invalidateCachedState()
		{
		}

		virtual void applyState(const State& s)
		{
		}
	};

	enum RrenderApi
	{
		RENDER_API_OPENGL,
		RENDER_API_DIRECTX11,
		RENDER_API_DIRECTX12,
		RENDER_API_METAL,
		//RENDER_API_VULCAN,
		RENDER_API_CUSTOM
	};

	RrenderApi getRenderApi();
	bool setRenderApi(RrenderApi api);
	bool setRenderApi(IRoxRenderApi* api);
	IRoxRenderApi& getApiInterface();
	IRoxRenderApi::State& getApiState();
}
