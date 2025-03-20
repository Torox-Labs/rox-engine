// Updated By the ROX_ENGINE
// Copyright © 2024 Torox Project
// Portions Copyright © 2013 nyan.developer@gmail.com (nya-engine)
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

#include "RoxRenderOpengl.h"

#ifdef _WIN32
#include "RoxRenderOpenglExt.h"
#endif

#include "RoxMemory/RoxTmpBuffers.h"
#include "RoxRenderObjects.h"
#include "RoxBitmap.h"
#include "RoxFBO.h"
#include "RoxLogger/RoxWarning.h"

namespace RoxRender
{
	// TODO: Include VAO to be a default functionality

	bool RoxRenderOpengl::isAvailable() const
	{
		// TODO: Will need more work on cross-platform support
		RoxLogger::log() << "OpenGL Active";
		return true;
	}

	namespace
	{
		IRoxRenderAPI::State applied_state;
		bool ignore_cache = true;
		bool ignore_cache_vp = true;
		RoxVBO::Layout applied_layout;
		bool was_fbo_without_color = false;
		int default_fbo_idx = -1;

		enum
		{
			VERTEX_ATTRIBUTE = 0,
			NORMAL_ATTRIBUTE = 1,
			COLOR_ATTRIBUTE = 2,
			TC0_ATTRIBUTE = 3,
			MAX_ATTRIBUTES = TC0_ATTRIBUTE + 16
		};

		int glCubeType(int side)
		{
			switch (side)
			{
			case RoxFBO::CUBE_POSITIVE_X: return GL_TEXTURE_CUBE_MAP_POSITIVE_X;
			case RoxFBO::CUBE_NEGATIVE_X: return GL_TEXTURE_CUBE_MAP_NEGATIVE_X;
			case RoxFBO::CUBE_POSITIVE_Y: return GL_TEXTURE_CUBE_MAP_POSITIVE_Y;
			case RoxFBO::CUBE_NEGATIVE_Y: return GL_TEXTURE_CUBE_MAP_NEGATIVE_Y;
			case RoxFBO::CUBE_POSITIVE_Z: return GL_TEXTURE_CUBE_MAP_POSITIVE_Z;
			case RoxFBO::CUBE_NEGATIVE_Z: return GL_TEXTURE_CUBE_MAP_NEGATIVE_Z;
			}
			return GL_TEXTURE_2D;
		}

		void setShader(int idx);

		struct ShaderObj
		{
		public:
			ShaderObj(): program(0)
			{
				memset(objects, 0, sizeof(objects));
				mat_mvp = mat_mv = mat_p = -1;
			}

			GLuint program, objects[RoxShader::PROGRAM_TYPES_COUNT];

			struct uniform : public RoxShader::Uniform
			{
				int handler, cache_idx;
			};

			std::vector<uniform> uniforms;
			std::vector<float> uniform_cache;
			int mat_mvp, mat_mv, mat_p;

			void release()
			{
				for (int i = 0; i < RoxShader::PROGRAM_TYPES_COUNT; ++i)
				{
					if (!objects[i])
						continue;

					glDetachShader(program, objects[i]);
					glDeleteShader(objects[i]);
				}

				if (program)
					glDeleteShader(program);

				*this = ShaderObj();
			}
		};

		RoxRenderObjects<ShaderObj> shaders;

		void setShader(int idx)
		{
			if (idx == applied_state.shader)
				return;

			if (idx < 0)
			{
				glUseProgram(0);
				applied_state.shader = -1;
				return;
			}

			ShaderObj& shdr = shaders.get(idx);
			glUseProgram(shdr.program);
			if (!shdr.program)
				applied_state.shader = -1;
			else
				applied_state.shader = idx;
		}

		int active_layer = -1;

		void glSelectMultitexLayer(int idx)
		{
			if (idx == active_layer)
				return;
			active_layer = idx;
			glActiveTexture(GL_TEXTURE0 + idx);
		}

		GLuint compileShader(RoxShader::PROGRAM_TYPE type, const char* src)
		{
			GLenum shader_type;
			switch (type)
			{
			case RoxShader::VERTEX: shader_type = GL_VERTEX_SHADER;
				break;
			case RoxShader::PIXEL: shader_type = GL_FRAGMENT_SHADER;
				break;
			case RoxShader::GEOMETRY: shader_type = GL_GEOMETRY_SHADER;
				break;
			default: return 0;
			}

			// Create Shader
			GLuint shader = glCreateShader(shader_type);
			glShaderSource(shader, 1, &src, 0);
			glCompileShader(shader);

			// Debug Shader
			int success = 1;
			glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
			if (!success)
			{
				// Display the Compiler Error
				const static char shader_type_name[][12] = { "VERTEX", "FRAGMENT", "GEOMETRY", "TESSELATION" };
				log() << "ERROR::" << shader_type_name[type] << "::SHADER::COMPILATION_FAILED\n";

				// Check the Log Length, and Display Message
				GLint log_length = 0;
				glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &log_length);
				if (log_length > 0)
				{
					std::string log_info(log_length, 0);
					glGetShaderInfoLog(shader, log_length, &log_length, &log_info[0]);
					log() << "Error: " << log_info.c_str() << "\n";
				}

				return 0;
			}

			return shader;
		}

	}

	int RoxRenderOpengl::createShader(const char* vertex_code, const char* fragment_code)
	{
		const int idx = shaders.add();
		ShaderObj& shdr = shaders.get(idx);

		shdr.program = glCreateProgram();
		if (!shdr.program)
		{
			log() << "Unable to create shader program object\n";
			shaders.remove(idx);
			return -1;
		}

		if(RoxShader::isBinaryShaderCachingEnabled())
			glProgramParameteri(shdr.program, GL_PROGRAM_BINARY_RETRIEVABLE_HINT, GL_TRUE);

		std::vector<std::string> fragment_vars;

		for (int i = 0; i < 2; ++i)
		{
			RoxShaderCodeParser parser(i == 0 ? vertex_code : fragment_code);
			RoxShader::PROGRAM_TYPE type = i == 0 ? RoxShader::VERTEX : RoxShader::PIXEL;

			if (i == 0 && strstr(vertex_code, "gl_Position") == 0)
			{
				for (int i = 0; i < parser.getOutCount(); ++i)
					fragment_vars.push_back(parser.getOut(i).name);
			}

			if (!parser.convertToGlsl3())
			{
				log() << "Unable to add shader program: cannot convert shader code to glsl3\n";
				log() << parser.getError() << "\n";
				shaders.remove(idx);
				return -1;
			}

			GLuint object = compileShader(type, parser.getCode());
			if (!object)
			{
				shaders.remove(idx);
				return -1;
			}

			glAttachShader(shdr.program, object);
			shdr.objects[type] = object;

			if (i == 0)
				for (int i = 0; i < parser.getAttributesCount(); ++i)
				{
					const RoxShaderCodeParser::Variable v = parser.getAttribute(i);
					if (v.name == "_rox_Vertex")
						glBindAttribLocation(shdr.program, VERTEX_ATTRIBUTE, v.name.c_str());
					else if (v.name == "_rox_Normal")
						glBindAttribLocation(shdr.program, NORMAL_ATTRIBUTE, v.name.c_str());
					else if (v.name == "_rox_Color")
						glBindAttribLocation(shdr.program, COLOR_ATTRIBUTE, v.name.c_str());
					else if (v.name.find("_rox_MultiTexCoord") == 0)
						glBindAttribLocation(
							shdr.program, TC0_ATTRIBUTE + v.idx, v.name.c_str());
				}

			for (int j = 0; j < parser.getUniformsCount(); ++j)
			{
				const RoxShaderCodeParser::Variable from = parser.getUniform(j);
				ShaderObj::uniform to;
				to.name = from.name;
				to.type = (RoxShader::UNIFORM_TYPE)from.type;
				to.array_size = from.array_size;

				// Check Predefined Uniforms
				if (to.type == RoxShader::UNIFORM_MAT4)
				{
					if (to.name == "_rox_ModelViewProjectionMatrix")
					{
						shdr.mat_mvp = 1;
						continue;
					}
					if (to.name == "_rox_ModelViewMatrix")
					{
						shdr.mat_mv = 1;
						continue;
					}
					if (to.name == "_rox_ProjectionMatrix")
					{
						shdr.mat_p = 1;
						continue;
					}
				}

				int idx = -1;
				for (int k = 0; k < (int)shdr.uniforms.size(); ++k)
				{
					if (shdr.uniforms[k].name == from.name)
					{
						idx = k;
						break;
					}
				}
				if (idx < 0) shdr.uniforms.push_back(to);
			}
		}

		if (!fragment_vars.empty() && isTransformFeedbackSupported())
		{
			std::vector<const GLchar*> vars;
			for (int i = 0; i < (int)fragment_vars.size(); ++i)
				vars.push_back(fragment_vars[i].c_str());
			glTransformFeedbackVaryings(shdr.program, (GLsizei)vars.size(), vars.data(),GL_INTERLEAVED_ATTRIBS);
		}

		glLinkProgram(shdr.program);

		int success = 1;
		glGetProgramiv(shdr.program,GL_LINK_STATUS, &success);
		if (!success)
		{
			log() << "Can't link shader\n";

			GLint log_length = 0;
			glGetProgramiv(shdr.program,GL_INFO_LOG_LENGTH, &log_length);
			if (log_length > 0)
			{
				std::string log_info(log_length, 0);
				glGetProgramInfoLog(shdr.program, log_length, &log_length, &log_info[0]);
				log() << "ERROR: " << log_info.c_str() << "\n";
			}

			shaders.remove(idx);
			return -1;
		}

		setShader(idx);

		for (size_t i = 0, layer = 0; i < shdr.uniforms.size(); ++i)
		{
			const ShaderObj::uniform& u = shdr.uniforms[i];
			if (u.type != RoxShader::UNIFORM_SAMPLER2D && u.type != RoxShader::UNIFORM_SAMPLER_CUBE)
				continue;

			int handler = glGetUniformLocation(shdr.program, u.name.c_str());
			if (handler >= 0)
				glUniform1i(handler, (int)layer);
			else
				log() << "ERROR: Unable to set shader sampler \'" << u.name.c_str() << "\': probably not found\n";

			++layer;
		}

		setShader(idx);

		if (shdr.mat_mvp > 0)
			shdr.mat_mvp = glGetUniformLocation(shdr.program, "_rox_ModelViewProjectionMatrix");
		else if (shdr.mat_mv > 0)
			shdr.mat_mv = glGetUniformLocation(shdr.program, "_rox_ModelViewMatrix");
		else if (shdr.mat_p > 0)
			shdr.mat_p = glGetUniformLocation(shdr.program, "_rox_ProjectionMatrix");

		for (int i = 0; i < (int)shdr.uniforms.size(); ++i)
			shdr.uniforms[i].handler = glGetUniformLocation(shdr.program, shdr.uniforms[i].name.c_str());

		int cache_size = 0;
		for (int i = 0; i < (int)shdr.uniforms.size(); ++i)
		{
			ShaderObj::uniform& u = shdr.uniforms[i];
			if (u.type == RoxShader::UNIFORM_SAMPLER2D || u.type == RoxShader::UNIFORM_SAMPLER_CUBE)
				continue;
			u.cache_idx = cache_size;
			cache_size += u.array_size * (u.type == RoxShader::UNIFORM_MAT4 ? 16 : 4);

			if (u.handler < 0)
				u.type = RoxShader::UNIFORM_NOT_FOUND;
		}
		shdr.uniform_cache.resize(cache_size);

		setShader(-1);

		return idx;
	}

	RoxRenderOpengl::uint RoxRenderOpengl::getUniformsCount(int shdr)
	{
		return (int)shaders.get(shdr).uniforms.size();
	}

	RoxShader::Uniform RoxRenderOpengl::getUniform(int shader, int idx) { return shaders.get(shader).uniforms[idx]; }

	void RoxRenderOpengl::removeShader(int shader)
	{
		if (applied_state.shader == shader)
		{
			glUseProgram(0);
			applied_state.shader = -1;
		}
		shaders.remove(shader);
	}

	//TODO: Uniform Buffers
	int RoxRenderOpengl::createUniformBuffer(int shader) { return shader; }

	void RoxRenderOpengl::setUniform(int shader, int idx, const float* buf, uint count)
	{
		ShaderObj& s = shaders.get(shader);

		float* cache = &s.uniform_cache[s.uniforms[idx].cache_idx];
		if (memcmp(cache, buf, count * sizeof(float)) == 0)
			return;
		memcpy(cache, buf, count * sizeof(float));

		setShader(shader);

		const int handler = s.uniforms[idx].handler;
		switch (s.uniforms[idx].type)
		{
		case RoxShader::UNIFORM_MAT4: glUniformMatrix4fv(handler, count / 16, false, buf);
			break;
		case RoxShader::UNIFORM_VEC4: glUniform4fv(handler, count / 4, buf);
			break;
		case RoxShader::UNIFORM_VEC3: glUniform3fv(handler, count / 3, buf);
			break;
		case RoxShader::UNIFORM_VEC2: glUniform2fv(handler, count / 2, buf);
			break;
		case RoxShader::UNIFORM_FLOAT: glUniform1fv(handler, count, buf);
			break;
		default: break;
		}
	}

	void RoxRenderOpengl::removeUniformBuffer(int uniform_buffer)
	{
	}

	namespace
	{
		int active_transform_feedback = 0;

		int getGLElementType(RoxVBO::ELEMENT_TYPE type)
		{
			switch (type)
			{
			case RoxVBO::TRIANGLES: return GL_TRIANGLES;;
			case RoxVBO::TRIANGLE_STRIP: return GL_TRIANGLE_STRIP;
			case RoxVBO::POINTS: return GL_POINTS;
			case RoxVBO::LINES: return GL_LINES;
			case RoxVBO::LINE_STRIP: return GL_LINE_STRIP;
			default: return -1;
			}

			return -1;
		}

		int getGLElementType(RoxVBO::VERTEX_ATRIB_TYPE type)
		{
			switch (type)
			{
			case RoxVBO::FLOAT_16: return GL_HALF_FLOAT;
			case RoxVBO::FLOAT_32: return GL_FLOAT;
			case RoxVBO::UINT_8: return GL_UNSIGNED_BYTE;
			}

			return GL_FLOAT;
		}

		int glUsage(RoxVBO::USAGE_HINT usage)
		{
			switch (usage)
			{
			case RoxVBO::STATIC_DRAW: return GL_STATIC_DRAW;
			case RoxVBO::DYNAMIC_DRAW: return GL_DYNAMIC_DRAW;
			case RoxVBO::STREAM_DRAW: return GL_STREAM_DRAW;
			}

			return GL_DYNAMIC_DRAW;
		}

		struct VertexBuffer
		{
			unsigned int id;

			unsigned int vertex_array_object;
			unsigned int active_vao_ibuf;
			RoxVBO::Layout layout;
			unsigned int stride;
			unsigned int count;
			RoxVBO::USAGE_HINT usage;

		public:
			void release()
			{
				if (vertex_array_object)
					glDeleteVertexArrays(1, &vertex_array_object);
				vertex_array_object = 0;

				if (id)
					glDeleteBuffers(1, &id);
				id = 0;
			}
		};

		RoxRenderObjects<VertexBuffer> vert_bufs;

		struct IndexBuffer
		{
			unsigned int id;
			RoxVBO::INDEX_SIZE type;
			unsigned int count;
			RoxVBO::USAGE_HINT usage;

		public:
			void release()
			{
				if (id)
					glDeleteBuffers(1, &id);
				id = 0;
			}
		};

		RoxRenderObjects<IndexBuffer> ind_bufs;
	}

	int RoxRenderOpengl::createVertexBuffer(const void* data, uint stride, uint count, RoxVBO::USAGE_HINT usage)
	{
		const int idx = vert_bufs.add();
		VertexBuffer& v = vert_bufs.get(idx);
		glGenBuffers(1, &v.id);
		glBindBuffer(GL_ARRAY_BUFFER, v.id);
		glBufferData(GL_ARRAY_BUFFER, count * stride, data, glUsage(usage));
		v.count = count;
		v.stride = stride;
		v.vertex_array_object = 0;
		v.active_vao_ibuf = 0;
		applied_state.vertex_buffer = -1;
		return idx;
	}

	void RoxRenderOpengl::setVertexLayout(int idx, RoxVBO::Layout layout)
	{
		VertexBuffer& v = vert_bufs.get(idx);
		v.layout = layout;

		if (v.vertex_array_object > 0)
		{
			glDeleteVertexArrays(1, &v.vertex_array_object);
			v.vertex_array_object = 0;
		}
	}

	void RoxRenderOpengl::updateVertexBuffer(int idx, const void* data)
	{
		VertexBuffer& v = vert_bufs.get(idx);
		//if(applied_state.vertex_buffer!=idx)
		{
			glBindVertexArray(0);
			glBindBuffer(GL_ARRAY_BUFFER, v.id);
			applied_state.vertex_buffer = -1;
		}
		glBufferData(GL_ARRAY_BUFFER, v.count * v.stride, 0, glUsage(v.usage)); //orphaning
		glBufferSubData(GL_ARRAY_BUFFER, 0, v.count * v.stride, data);
		//if(applied_state.vertex_buffer!=idx)
		glBindBuffer(GL_ARRAY_BUFFER, 0);
	}

	bool RoxRenderOpengl::getVertexData(int idx, void* data)
	{
		const VertexBuffer& v = vert_bufs.get(idx);
		//if(applied_state.vertex_buffer!=idx)
		{
			glBindVertexArray(0);
			glBindBuffer(GL_ARRAY_BUFFER, v.id);
			applied_state.vertex_buffer = -1;
		}
		glGetBufferSubData(GL_ARRAY_BUFFER, 0, v.stride * v.count, data);
		//if(applied_state.vertex_buffer!=idx)
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		
		return true;
	}

	void RoxRenderOpengl::removeVertexBuffer(int idx)
	{
		if (active_transform_feedback == idx)
		{
			active_transform_feedback = 0;
			glBindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 0, 0);
		}

		if (applied_state.vertex_buffer == idx)
		{
			glBindVertexArray(0);
			glBindBuffer(GL_ARRAY_BUFFER, 0);
			applied_state.vertex_buffer = -1;
		}
		vert_bufs.remove(idx);
	}

	int RoxRenderOpengl::createIndexBuffer(const void* data, RoxVBO::INDEX_SIZE size, uint indices_count,
	                                       RoxVBO::USAGE_HINT usage)
	{
		const int idx = ind_bufs.add();
		IndexBuffer& i = ind_bufs.get(idx);

		glBindVertexArray(0);
		applied_state.vertex_buffer = -1;

		glGenBuffers(1, &i.id);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, i.id);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, size * indices_count, data, glUsage(usage));
		i.type = size;
		i.count = indices_count;
		i.usage = usage;
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
		
		return idx;
	}

	void RoxRenderOpengl::updateIndexBuffer(int idx, const void* data)
	{
		const IndexBuffer& i = ind_bufs.get(idx);

		glBindVertexArray(0);
		applied_state.vertex_buffer = -1;
		
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, i.id);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, i.count * i.type, data, glUsage(i.usage));
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	}

	bool RoxRenderOpengl::getIndexData(int idx, void* data)
	{
		const IndexBuffer& i = ind_bufs.get(idx);

		glBindVertexArray(0);
		applied_state.vertex_buffer = -1;

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, i.id);
		glGetBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, i.type * i.count, data);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	
		return true;
	}

	void RoxRenderOpengl::removeIndexBuffer(int idx)
	{
		if (applied_state.index_buffer == idx)
		{
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
			applied_state.index_buffer = -1;
		}
		ind_bufs.remove(idx);
	}

	namespace
	{
		const unsigned int cube_faces[] = {
			GL_TEXTURE_CUBE_MAP_POSITIVE_X,GL_TEXTURE_CUBE_MAP_NEGATIVE_X,
			GL_TEXTURE_CUBE_MAP_POSITIVE_Y,GL_TEXTURE_CUBE_MAP_NEGATIVE_Y,
			GL_TEXTURE_CUBE_MAP_POSITIVE_Z,GL_TEXTURE_CUBE_MAP_NEGATIVE_Z
		};

		void gl_setup_filtration(int target, bool has_mips, RoxTexture::FILTER minif, RoxTexture::FILTER magnif,
		                         RoxTexture::FILTER mip)
		{
			glTexParameteri(target,GL_TEXTURE_MAG_FILTER,
			                magnif == RoxTexture::FILTER_NEAREST ? GL_NEAREST : GL_LINEAR);

			GLint FILTER;
			if (has_mips)
			{
				if (minif == RoxTexture::FILTER_LINEAR)
				{
					if (mip == RoxTexture::FILTER_NEAREST)
						FILTER = GL_NEAREST_MIPMAP_NEAREST;
					else
						FILTER = GL_NEAREST_MIPMAP_LINEAR;
				}
				else
				{
					if (mip == RoxTexture::FILTER_NEAREST)
						FILTER = GL_LINEAR_MIPMAP_NEAREST;
					else
						FILTER = GL_LINEAR_MIPMAP_LINEAR;
				}
			}
			else if (minif == RoxTexture::FILTER_NEAREST)
				FILTER = GL_NEAREST;
			else
				FILTER = GL_LINEAR;

			glTexParameteri(target,GL_TEXTURE_MIN_FILTER, FILTER);
		}

		void gl_generate_mips_pre(int gl_type)
		{
#if defined GL_GENERATE_MIPMAP && !defined OPENGL3
        glTexParameteri(gl_type,GL_GENERATE_MIPMAP,GL_TRUE);
#endif
		}

		void gl_generate_mips_post(int gl_type)
		{
#if defined GL_GENERATE_MIPMAP && !defined OPENGL3
        glTexParameteri(gl_type,GL_GENERATE_MIPMAP,GL_FALSE);
#else
			::glGenerateMipmap(gl_type);
#endif
		}

		bool gl_get_format(RoxTexture::COLOR_FORMAT format, unsigned int& source_format, unsigned int& gl_format,
		                   unsigned int& precision)
		{
			precision = GL_UNSIGNED_BYTE;
			switch (format)
			{
			case RoxTexture::COLOR_RGB: source_format = gl_format = GL_RGB;
				break; //in es stored internally as rgba
			case RoxTexture::COLOR_RGBA: source_format = gl_format = GL_RGBA;
				break;
#ifdef USE_BGRA
            case RoxTexture::COLOR_BGRA: source_format=GL_RGBA; gl_format=GL_BGRA; break;
#endif
			case RoxTexture::GREYSCALE: source_format = gl_format = GL_LUMINANCE;
				break;
#ifdef OPENGL3
            case RoxTexture::COLOR_R32F: source_format=GL_R32F; gl_format=GL_RED; precision=GL_FLOAT; break;
            case RoxTexture::COLOR_RGB32F: source_format=GL_RGB32F; gl_format=GL_RGB; precision=GL_FLOAT; break;
            case RoxTexture::COLOR_RGBA32F: source_format=GL_RGBA32F; gl_format=GL_RGBA; precision=GL_FLOAT; break;
#else
			case RoxTexture::COLOR_R32F: source_format = GL_R32F;
				gl_format = GL_RED;
				precision = GL_FLOAT;
				break;
			case RoxTexture::COLOR_RGB32F: source_format = GL_RGB32F_ARB;
				gl_format = GL_RGB;
				precision = GL_FLOAT;
				break;
			case RoxTexture::COLOR_RGBA32F: source_format = GL_RGBA32F_ARB;
				gl_format = GL_RGBA;
				precision = GL_FLOAT;
				break;
#endif
			case RoxTexture::DEPTH16: source_format = GL_DEPTH_COMPONENT16;
				gl_format = GL_DEPTH_COMPONENT;
				break;
			case RoxTexture::DEPTH24: source_format = GL_DEPTH_COMPONENT24;
				gl_format = GL_DEPTH_COMPONENT;
				break;
			case RoxTexture::DEPTH32: source_format = GL_DEPTH_COMPONENT32;
				gl_format = GL_DEPTH_COMPONENT;
				break;
			case RoxTexture::DXT1: source_format = gl_format = GL_COMPRESSED_RGBA_S3TC_DXT1_EXT;
				break;
			case RoxTexture::DXT3: source_format = gl_format = GL_COMPRESSED_RGBA_S3TC_DXT3_EXT;
				break;
			case RoxTexture::DXT5: source_format = gl_format = GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
				break;

			default: return false;
			};

			return true;
		}

		struct tex_obj
		{
			unsigned int tex_id, gl_type, gl_format, gl_precision;
			bool has_mip;

		public:
			void release()
			{
				if (tex_id)
					glDeleteTextures(1, &tex_id);
				tex_id = 0;
			}
		};

		RoxRenderObjects<tex_obj> textures;

		void set_texture(int idx, int layer)
		{
			glSelectMultitexLayer(layer);

			const int prev_tex = applied_state.textures[layer];
			applied_state.textures[layer] = idx;
			if (idx < 0)
			{
				if (prev_tex >= 0)
					glBindTexture(textures.get(prev_tex).gl_type, 0);
				return;
			}

			const tex_obj& t = textures.get(idx);
			glBindTexture(t.gl_type, t.tex_id);
		}

		int create_texture_(const void* data_a[6], bool is_cubemap, unsigned int width, unsigned int height,
		                    RoxTexture::COLOR_FORMAT& format, int mip_count)
		{
			const int idx = textures.add();
			tex_obj& t = textures.get(idx);
			glGenTextures(1, &t.tex_id);
			t.gl_type = is_cubemap ? GL_TEXTURE_CUBE_MAP : GL_TEXTURE_2D;
			set_texture(idx, 0);

			const unsigned int source_bpp = RoxTexture::getFormatBpp(format);
#ifdef MANUAL_MIPMAP_GENERATION
        const bool bad_alignment=(source_bpp/8)%4!=0;
#else
			const bool bad_alignment = (width * source_bpp / 8) % 4 != 0;
#endif
			if (bad_alignment)
				glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

			t.has_mip = (mip_count > 1 || mip_count < 0);

			if (t.gl_type == GL_TEXTURE_CUBE_MAP)
			{
				glTexParameteri(GL_TEXTURE_CUBE_MAP,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);
				glTexParameteri(GL_TEXTURE_CUBE_MAP,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);
				glTexParameteri(GL_TEXTURE_CUBE_MAP,GL_TEXTURE_WRAP_R,GL_CLAMP_TO_EDGE);
			}

			const bool is_pvrtc = format == RoxTexture::PVR_RGB2B || format == RoxTexture::PVR_RGBA2B || format ==
				RoxTexture::PVR_RGB4B || format == RoxTexture::PVR_RGBA4B;
			if (is_pvrtc)
				gl_setup_filtration(t.gl_type, t.has_mip, RoxTexture::FILTER_LINEAR, RoxTexture::FILTER_LINEAR,
				                    RoxTexture::FILTER_NEAREST);

#ifdef OPENGL3
        if(format==RoxTexture::GREYSCALE)
        {
            const int swizzle[]={GL_RED,GL_RED,GL_RED,GL_ONE};
            glTexParameteriv(t.gl_type,GL_TEXTURE_SWIZZLE_RGBA,swizzle);
        }
#endif

			if (mip_count > 1) //is_platform_restrictions_ignored() &&
				glTexParameteri(t.gl_type,GL_TEXTURE_MAX_LEVEL, mip_count - 1);

#ifndef MANUAL_MIPMAP_GENERATION
			if (t.has_mip && mip_count < 0)
				gl_generate_mips_pre(t.gl_type);
#endif
			unsigned int source_format = 0;
			gl_get_format(format, source_format, t.gl_format, t.gl_precision);

			for (int j = 0; j < (is_cubemap ? 6 : 1); ++j)
			{
				const unsigned char* data_pointer = data_a ? (const unsigned char*)data_a[j] : 0;
				unsigned int w = width, h = height;
				const int gl_type = is_cubemap ? cube_faces[j] : GL_TEXTURE_2D;

#ifdef MANUAL_MIPMAP_GENERATION
            if(mip_count<0 && data_pointer)
            {
                nya_memory::tmp_buffer_scoped buf(w*h*(source_bpp/8)/2);
                for(int i=0;;++i,w=w>1?w/2:1,h=h>1?h/2:1)
                {
                    glTexImage2D(gl_type,i,source_format,w,h,0,t.gl_format,t.gl_precision,data_pointer);
                    if(w==1 && h==1)
                        break;

                    bitmap_downsample2x(data_pointer,w,h,source_bpp/8,(unsigned char *)buf.get_data());
                    data_pointer=(unsigned char *)buf.get_data();
                }
            }
            else
#endif
				{
					for (int i = 0; i < (mip_count <= 0 ? 1 : mip_count); ++i, w = w > 1 ? w / 2 : 1, h =
					     h > 1 ? h / 2 : 1)
					{
						unsigned int size = 0;
						if (format < RoxTexture::DXT1)
						{
							size = w * h * (source_bpp / 8);
							glTexImage2D(gl_type, i, source_format, w, h, 0, t.gl_format, t.gl_precision, data_pointer);
						}
						else
						{
							if (is_pvrtc)
							{
								if (format == RoxTexture::PVR_RGB2B || format == RoxTexture::PVR_RGBA2B)
									size = ((w > 16 ? w : 16) * (h > 8 ? h : 8) * 2 + 7) / 8;
								else
									size = ((w > 8 ? w : 8) * (h > 8 ? h : 8) * 4 + 7) / 8;

								if (size < 32)
									break;
							}
							else
								size = (w > 4 ? w : 4) / 4 * (h > 4 ? h : 4) / 4 * source_bpp * 2;

							::glCompressedTexImage2D(gl_type, i, t.gl_format, w, h, 0, size, data_pointer);
						}
						data_pointer += size;
					}
				}
			}

			if (bad_alignment)
				glPixelStorei(GL_UNPACK_ALIGNMENT, 4);

#ifndef MANUAL_MIPMAP_GENERATION
			if (t.has_mip && mip_count < 0)
				gl_generate_mips_post(t.gl_type);
#endif
			return idx;
		}
	}

	int RoxRenderOpengl::createTexture(const void* data, uint width, uint height, RoxTexture::COLOR_FORMAT& format,
	                                   int mip_count)
	{
		const void* data_a[6] = {data};
		return create_texture_(data_a, false, width, height, format, mip_count);
	}

	int RoxRenderOpengl::createCubemap(const void* data[6], uint width, RoxTexture::COLOR_FORMAT& format, int mip_count)
	{
		return create_texture_(data, true, width, width, format, mip_count);
	}

	void RoxRenderOpengl::updateTexture(int idx, const void* data, uint x, uint y, uint width, uint height, int mip)
	{
		const tex_obj& t = textures.get(idx);
		set_texture(idx, 0);

		if (t.has_mip && mip < 0)
			gl_generate_mips_pre(t.gl_type);

		glTexSubImage2D(t.gl_type, mip < 0 ? 0 : mip, x, y, width, height, t.gl_format, t.gl_precision, data);

		if (t.has_mip && mip < 0)
		{
			gl_generate_mips_post(t.gl_type);

		}
	}

	void RoxRenderOpengl::setTextureWrap(int idx, RoxTexture::WRAP s, RoxTexture::WRAP t)
	{
		const tex_obj& tex = textures.get(idx);
		set_texture(idx, 0);

		const RoxTexture::WRAP wraps[] = {s, t};
		const GLint pnames[] = {GL_TEXTURE_WRAP_S,GL_TEXTURE_WRAP_T};
		for (int i = 0; i < 2; ++i)
		{
			switch (wraps[i])
			{
			case RoxTexture::WRAP_CLAMP: glTexParameteri(tex.gl_type, pnames[i],GL_CLAMP_TO_EDGE);
				break;
			case RoxTexture::WRAP_REPEAT: glTexParameteri(tex.gl_type, pnames[i],GL_REPEAT);
				break;
			case RoxTexture::WRAP_REPEAT_MIRROR: glTexParameteri(tex.gl_type, pnames[i],GL_MIRRORED_REPEAT);
				break;
			}
		}
	}

	void RoxRenderOpengl::setTextureFilter(int idx, RoxTexture::FILTER minification, RoxTexture::FILTER magnification,
	                                       RoxTexture::FILTER mipmap, uint aniso)
	{
		static int max_aniso = -1;
		if (max_aniso < 0)
		{
			GLfloat f = 0.0f;
			glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &f);
			max_aniso = int(f);
		}

		if ((int)aniso > max_aniso)
			aniso = (uint)max_aniso;
		if (!aniso)
			aniso = 1;

		const tex_obj& t = textures.get(idx);
		set_texture(idx, 0);
		glTexParameterf(t.gl_type,GL_TEXTURE_MAX_ANISOTROPY_EXT, float(aniso));
		gl_setup_filtration(t.gl_type, t.has_mip, minification, magnification, mipmap);
	}

	bool RoxRenderOpengl::getTextureData(int texture, uint x, uint y, uint w, uint h, void* data)
	{
		const tex_obj& t = textures.get(texture);

		//compressed formats are not supported
		switch (t.gl_format)
		{
		case GL_COMPRESSED_RGBA_S3TC_DXT1_EXT:
		case GL_COMPRESSED_RGBA_S3TC_DXT3_EXT:
		case GL_COMPRESSED_RGBA_S3TC_DXT5_EXT:
			return false;
		}

		glViewport(0, 0, x + w, y + h);

		uint tmp_fbo;
		glGenFramebuffers(1, &tmp_fbo);
		glBindFramebuffer(GL_FRAMEBUFFER, tmp_fbo);

		if (t.gl_type == GL_TEXTURE_CUBE_MAP)
		{
			unsigned int size = (w - x) * (h - y);
			if (t.gl_precision == GL_FLOAT)
				size *= 4;

			switch (t.gl_format)
			{
			case GL_RGB: size *= 3;
				break;

			case GL_RGBA:
#ifdef USE_BGRA
			case GL_BGRA:
#endif
			case GL_COMPRESSED_RGBA_S3TC_DXT1_EXT:
			case GL_COMPRESSED_RGBA_S3TC_DXT3_EXT:
			case GL_COMPRESSED_RGBA_S3TC_DXT5_EXT:
				size *= 4;
				break;
			}

			for (int i = 0; i < 6; ++i)
			{
				glFramebufferTexture2D(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT0, glCubeType(i), t.tex_id, 0);
				glReadPixels(x, y, w, h, t.gl_format, t.gl_precision, (char*)data + size * i);
			}
		}
		else
		{
			glFramebufferTexture2D(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT0, t.gl_type, t.tex_id, 0);
			glReadPixels(x, y, w, h, t.gl_format, t.gl_precision, data);
		}

		glBindFramebuffer(GL_FRAMEBUFFER, default_fbo_idx);
		glDeleteFramebuffers(1, &tmp_fbo);

		applied_state.target = -1;
		glViewport(applied_state.viewport.x, applied_state.viewport.y, applied_state.viewport.width,
		           applied_state.viewport.height);
		return true;
	}

	void RoxRenderOpengl::removeTexture(int texture)
	{
		for (int i = 0; i < State::max_layers; ++i)
		{
			if (applied_state.textures[i] != texture)
				continue;

			applied_state.textures[i] = -1;
			glSelectMultitexLayer(i);
			const tex_obj& t = textures.get(texture);
			glBindTexture(t.gl_type, 0);
		}
		textures.remove(texture);
	}

	unsigned int RoxRenderOpengl::getMaxTextureDimension()
	{
		static unsigned int max_tex_size = 0;
		if (!max_tex_size)
		{
			GLint texSize;
			glGetIntegerv(GL_MAX_TEXTURE_SIZE, &texSize);
			max_tex_size = texSize;
		}

		return max_tex_size;
	}

	bool RoxRenderOpengl::isTextureFormatSupported(RoxTexture::COLOR_FORMAT format)
	{
		unsigned int source_format, gl_format, precision;
		return gl_get_format(format, source_format, gl_format, precision);
	}

	//target

	namespace
	{
		struct ms_buffer
		{
			unsigned int RoxFbo, buf;
			unsigned int width, height, samples, format;

			void create(unsigned int w, unsigned int h, unsigned int f, unsigned int s)
			{
				if (!glRenderbufferStorageMultisample)
					return;

				if (!w || !h || s < 1)
				{
					release();
					return;
				}

				if (w == width && h == height && s == samples && f == format)
					return;

				glGenRenderbuffers(1, &buf);
				glBindRenderbuffer(GL_RENDERBUFFER, buf);
				glRenderbufferStorageMultisample(GL_RENDERBUFFER, s, f, w, h);
				glBindRenderbuffer(GL_RENDERBUFFER, 0);
				glGenFramebuffers(1, &RoxFbo);

				width = w, height = h, samples = s, format = f;
			}

			void resolve(int from, tex_obj& tex, int cubemap_side, int attachment_idx) const;

			void release()
			{
				if (buf) ::glDeleteRenderbuffers(1, &buf);
				if (RoxFbo) ::glDeleteFramebuffers(1, &RoxFbo);
				*this = ms_buffer();
			}

			ms_buffer(): buf(0), width(0), height(0), samples(0), RoxFbo(0)
			{
			}
		};

		struct fbo_obj
		{
			struct attachment
			{
				int tex_idx, cubemap_side;
				ms_buffer multisample;

				attachment(): tex_idx(-1), cubemap_side(-1)
				{
				}
			};

			std::vector<attachment> color_attachments;
			unsigned int id;
			int depth_tex_idx;
			ms_buffer multisample_depth;
			bool depth_only;

			void release()
			{
				for (size_t i = 0; i < color_attachments.size(); ++i)
					color_attachments[i].multisample.release();
				multisample_depth.release();
				if (id)
					glDeleteFramebuffers(1, &id);
				*this = fbo_obj();
			}
		};

		RoxRenderObjects<fbo_obj> fbos;

		void ms_buffer::resolve(int from, tex_obj& tex, int cubemap_side, int attachment_idx) const
		{
			if (!buf)
				return;

			const int gl_type = cubemap_side < 0 ? tex.gl_type : glCubeType(cubemap_side);
			glBindFramebuffer(GL_FRAMEBUFFER, RoxFbo);
			glFramebufferTexture2D(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT0, gl_type, tex.tex_id, 0);
			glBindFramebuffer(GL_FRAMEBUFFER, default_fbo_idx);


			{
				glBindFramebuffer(GL_READ_FRAMEBUFFER, from);
				glBindFramebuffer(GL_DRAW_FRAMEBUFFER, RoxFbo);
				glReadBuffer(GL_COLOR_ATTACHMENT0 + attachment_idx);
				glBlitFramebuffer(0, 0, width, height, 0, 0, width, height,GL_COLOR_BUFFER_BIT,GL_NEAREST);
				glReadBuffer(GL_COLOR_ATTACHMENT0);
				glBindFramebuffer(GL_READ_FRAMEBUFFER, default_fbo_idx);
				glBindFramebuffer(GL_DRAW_FRAMEBUFFER, default_fbo_idx);
			}

			applied_state.target = -1;
		}
	}

	int RoxRenderOpengl::createTarget(uint width, uint height, uint samples, const int* attachment_textures,
	                                  const int* attachment_sides, uint attachment_count, int depth_texture)
	{
		if (default_fbo_idx < 0)
			glGetIntegerv(GL_FRAMEBUFFER_BINDING, &default_fbo_idx);

		int idx = fbos.add();
		fbo_obj& f = fbos.get(idx);

		glGenFramebuffers(1, &f.id);
		glBindFramebuffer(GL_FRAMEBUFFER, f.id);

		bool has_color = false;
		f.color_attachments.resize(attachment_count);
		for (uint i = 0; i < attachment_count; ++i)
		{
			if (attachment_textures[i] < 0)
				continue;

			f.color_attachments[i].tex_idx = attachment_textures[i];
			f.color_attachments[i].cubemap_side = attachment_sides[i];
			has_color = true;

			const tex_obj& t = textures.get(attachment_textures[i]);
			if (samples > 1)
			{
				f.color_attachments[i].multisample.create(width, height, t.gl_format, samples);
				glFramebufferRenderbuffer(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT0 + i,GL_RENDERBUFFER,
				                            f.color_attachments[i].multisample.buf);
			}
			else
			{
				const int gl_type = attachment_sides[i] < 0 ? t.gl_type : glCubeType(attachment_sides[i]);
				glFramebufferTexture2D(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT0 + i, gl_type, t.tex_id, 0);
			}
		}

		if (depth_texture >= 0)
		{
			const tex_obj& t = textures.get(depth_texture);
			if (samples > 1)
			{
				f.multisample_depth.create(width, height, t.gl_format, samples);
				glFramebufferRenderbuffer(GL_FRAMEBUFFER,GL_DEPTH_ATTACHMENT,GL_RENDERBUFFER,
				                            f.multisample_depth.buf);
			}
			else
				glFramebufferTexture2D(GL_FRAMEBUFFER,GL_DEPTH_ATTACHMENT,GL_TEXTURE_2D, t.tex_id, 0);
			f.depth_tex_idx = depth_texture;
		}
		else
			f.depth_tex_idx = -1;

		f.depth_only = !has_color && depth_texture >= 0;

		glBindFramebuffer(GL_FRAMEBUFFER, applied_state.target >= 0
			                                    ? fbos.get(applied_state.target).id
			                                    : default_fbo_idx);
		return idx;
	}

	void RoxRenderOpengl::resolveTarget(int idx)
	{
		const fbo_obj& f = fbos.get(idx);
		for (int i = 0; i < (int)f.color_attachments.size(); ++i)
		{
			const fbo_obj::attachment& a = f.color_attachments[i];
			if (a.tex_idx < 0)
				continue;

			tex_obj& tex = textures.get(a.tex_idx);
			a.multisample.resolve(f.id, tex, a.cubemap_side, int(i));

			if (tex.has_mip)
			{
				glSelectMultitexLayer(0);
				glBindTexture(tex.gl_type, tex.tex_id);
				applied_state.textures[0] = tex.tex_id;
				glGenerateMipmap(tex.gl_type);
			}
		}
	}

	void RoxRenderOpengl::removeTarget(int idx)
	{
		if (applied_state.target == idx)
		{
			glBindFramebuffer(GL_FRAMEBUFFER, default_fbo_idx);
			applied_state.target = -1;
		}
		return fbos.remove(idx);
	}

	unsigned int RoxRenderOpengl::getMaxTargetAttachments()
	{
		static int max_attachments = -1;
		if (max_attachments < 0)
			glGetIntegerv(GL_MAX_COLOR_ATTACHMENTS, &max_attachments);
		if (max_attachments < 0)
			max_attachments = 1;
		return max_attachments;
	}

	unsigned int RoxRenderOpengl::getMaxTargetMsaa()
	{
		static int max_ms = -1;

		if (max_ms < 0)
			glGetIntegerv(GL_MAX_SAMPLES, &max_ms);

		return max_ms > 1 ? max_ms : 1;
	}

	static void apply_viewport_state(IRoxRenderAPI::ViewportState s)
	{
		if (ignore_cache_vp)
		{
			if (default_fbo_idx < 0)
				glGetIntegerv(GL_FRAMEBUFFER_BINDING, &default_fbo_idx);
		}

		if (s.target != applied_state.target || ignore_cache_vp)
		{
			glBindFramebuffer(GL_FRAMEBUFFER, s.target >= 0 ? fbos.get(s.target).id : default_fbo_idx);

			const bool no_color = s.target >= 0 && fbos.get(s.target).depth_only;
			if (s.target >= 0 && (no_color != was_fbo_without_color || ignore_cache_vp))
			{
				const int buffer = no_color ? GL_NONE : GL_COLOR_ATTACHMENT0;
				glDrawBuffer(buffer);
				glReadBuffer(buffer);
				was_fbo_without_color = no_color;
			}
		}

		if (applied_state.viewport != s.viewport || ignore_cache_vp)
			glViewport(s.viewport.x, s.viewport.y, s.viewport.width, s.viewport.height);

		if (memcmp(applied_state.clear_color, s.clear_color, sizeof(applied_state.clear_color)) != 0 || ignore_cache_vp)
			glClearColor(s.clear_color[0], s.clear_color[1], s.clear_color[2], s.clear_color[3]);

		if (applied_state.clear_depth != s.clear_depth || ignore_cache_vp)
		{
			glClearDepth(s.clear_depth);
		}

		if (s.scissor_enabled != applied_state.scissor_enabled || ignore_cache_vp)
		{
			if (s.scissor_enabled)
				glEnable(GL_SCISSOR_TEST);
			else
				glDisable(GL_SCISSOR_TEST);
		}

		if (s.scissor != applied_state.scissor)
			glScissor(s.scissor.x, s.scissor.y, s.scissor.width, s.scissor.height);

		*(IRoxRenderAPI::ViewportState*)&applied_state = s;
		ignore_cache_vp = false;
	}

	inline GLenum gl_blend_mode(Blend::MODE m)
	{
		switch (m)
		{
		case Blend::ZERO: return GL_ZERO;
		case Blend::ONE: return GL_ONE;
		case Blend::SRC_COLOR: return GL_SRC_COLOR;
		case Blend::INV_SRC_COLOR: return GL_ONE_MINUS_SRC_COLOR;
		case Blend::SRC_ALPHA: return GL_SRC_ALPHA;
		case Blend::INV_SRC_ALPHA: return GL_ONE_MINUS_SRC_ALPHA;
		case Blend::DST_COLOR: return GL_DST_COLOR;
		case Blend::INV_DST_COLOR: return GL_ONE_MINUS_DST_COLOR;
		case Blend::DST_ALPHA: return GL_DST_ALPHA;
		case Blend::INV_DST_ALPHA: return GL_ONE_MINUS_DST_ALPHA;
		}

		return GL_ONE;
	}

	void RoxRenderOpengl::clear(const ViewportState& s, bool color, bool depth, bool stencil)
	{
		apply_viewport_state(s);

		GLbitfield mode = 0;
		if (color)
		{
			mode |= GL_COLOR_BUFFER_BIT;
			if (!applied_state.color_write)
			{
				glColorMask(true, true, true, true);
				applied_state.color_write = true;
			}
		}

		if (depth)
		{
			mode |= GL_DEPTH_BUFFER_BIT;
			if (!applied_state.zwrite)
			{
				glDepthMask(true);
				applied_state.zwrite = true;
			}
		}

		if (stencil)
			mode |= GL_STENCIL_BUFFER_BIT;

		glClear(mode);
	}

	void RoxRenderOpengl::invalidateCachedState()
	{
		ignore_cache = true;
		ignore_cache_vp = true;

		applied_state.index_buffer = applied_state.vertex_buffer = -1;
		applied_state.shader = applied_state.uniform_buffer = -1;
		active_layer = -1;
		for (int i = 0; i < State::max_layers; ++i)
		{
			applied_state.textures[i] = -1;
			glSelectMultitexLayer(i);
			glBindTexture(GL_TEXTURE_2D, 0);
			glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
		}
	}

	int RoxRenderOpengl::setProgramBinaryShader(const char* vertex_code, const char* fragment_code, RoxCompiledShader& cmp_shdr)
	{
		const int idx = shaders.add();
		ShaderObj& shdr = shaders.get(idx);

		shdr.program = glCreateProgram();
		if (!shdr.program)
		{
			log() << "ERROR: Unable to create shader program object.\n";
			return -1;
		}

		const void* binary_data = cmp_shdr.getData();
		size_t binary_size = cmp_shdr.getSize();

		if (!binary_data || binary_size < sizeof(GLenum))
		{
			log() << "ERROR: Invalid shader binary data format or size.\n";
			return -1;
		}

		GLenum binary_format = *reinterpret_cast<const GLenum*>(binary_data);
		const void* program_binary = reinterpret_cast<const char*>(binary_data) + sizeof(GLenum);
		GLint program_binary_size = static_cast<GLint>(binary_size - sizeof(GLenum));

		// Load Binary to Opengl
		glProgramBinary(shdr.program, binary_format, program_binary, program_binary_size);

		glValidateProgram(shdr.program);
		GLint validate_status;
		glGetProgramiv(shdr.program, GL_VALIDATE_STATUS, &validate_status);
		if (!validate_status)
		{
			GLint log_length = 0;
			glGetProgramiv(shdr.program, GL_INFO_LOG_LENGTH, &log_length);
			if (log_length > 0)
			{
				std::string log_info(log_length, 0);
				glGetProgramInfoLog(shdr.program, log_length, nullptr, &log_info[0]);
				log() << "ERROR: program validation failed: " << log_info << "\n";
			}

			return -1;
		}

		GLint success;
		glGetProgramiv(shdr.program, GL_LINK_STATUS, &success);
		if (!success)
		{
			log() << "ERROR: Can't link program shader binary";

			GLint log_length = 0;
			glGetProgramiv(shdr.program, GL_INFO_LOG_LENGTH, &log_length);
			if (log_length> 0)
			{
				std::string log_info(log_length, 0);
				glGetProgramInfoLog(shdr.program, log_length, nullptr, &log_info[0]);
				log() << "ERROR: " << log_info.c_str() << "\n";
			}

			return -1;
		}

		// Set Uniforms
		for (int i = 0; i < 2; i++)
		{
			RoxShaderCodeParser parser(i == 0 ? vertex_code : fragment_code);
			RoxShader::PROGRAM_TYPE type = i == 0 ? RoxShader::VERTEX : RoxShader::PIXEL;

			if (!parser.convertToGlsl3())
			{
				log() << "Unable to add shader program: cannot convert shader code to glsl3\n";
				log() << parser.getError() << "\n";
				shaders.remove(idx);
				return -1;
			}

			// Bind Uniforms
			for (int j = 0; j < parser.getUniformsCount(); j++)
			{
				const RoxShaderCodeParser::Variable from = parser.getUniform(j);
				ShaderObj::uniform to;
				to.name = from.name;
				to.type = (RoxShader::UNIFORM_TYPE)from.type;
				to.array_size = from.array_size;

				// Check Predefined Uniforms
				if (to.type == RoxShader::UNIFORM_MAT4)
				{
					if (to.name == "_rox_ModelViewProjectionMatrix")
					{
						shdr.mat_mvp = 1;
						continue;
					}
					if (to.name == "_rox_ModelViewMatrix")
					{
						shdr.mat_mv = 1;
						continue;
					}
					if (to.name == "_rox_ProjectionMatrix")
					{
						shdr.mat_p = 1;
						continue;
					}
				}

				// Check if the Unifom is already in the list
				int idx = -1;
				for (int k = 0; k < (int)shdr.uniforms.size(); ++k)
					if (shdr.uniforms[k].name == from.name)
					{
						idx = k;
						break;
					}

				if (idx < 0) shdr.uniforms.push_back(to);
			}
			
			setShader(idx);

			// Bind Shader Sampler Uniforms
			for (size_t i = 0, layer = 0; i < shdr.uniforms.size(); ++i) {
				const ShaderObj::uniform& u = shdr.uniforms[i];
				if (u.type != RoxShader::UNIFORM_SAMPLER2D || u.type != RoxShader::UNIFORM_SAMPLER_CUBE)
					continue;

				int handler = glGetUniformLocation(shdr.program, u.name.c_str());
				if (handler >= 0)
					glUniform1i(handler, (int)layer);
				else
					log() << "ERROR: Unable to set shader sampler \'" << u.name.c_str() << "\': probably not found\n";
				
				layer++;
			}

			if (shdr.mat_mvp > 0)
				shdr.mat_mvp = glGetUniformLocation(shdr.program, "_rox_ModelViewProjectionMatrix");
			else if (shdr.mat_mv > 0)
				shdr.mat_mv = glGetUniformLocation(shdr.program, "_rox_ModelViewMatrix");
			else if (shdr.mat_p > 0)
				shdr.mat_p = glGetUniformLocation(shdr.program, "_rox_ProjectionMatrix");

			for (int i = 0; i < (int)shdr.uniforms.size(); i++)
				shdr.uniforms[i].handler = glGetUniformLocation(shdr.program, shdr.uniforms[i].name.c_str());

			// Cache Uniforms 
			// "Holds the last value we sent to the shader for each non-sampler uniform"
			int cache_size = 0;
			for (int i = 0; i < (int)shdr.uniforms.size(); i++)
			{
				ShaderObj::uniform& u = shdr.uniforms[i];
				if (u.type == RoxShader::UNIFORM_SAMPLER2D || u.type == RoxShader::UNIFORM_SAMPLER_CUBE)
					continue;
				u.cache_idx = cache_size;
				cache_size += u.array_size * (u.type == RoxShader::UNIFORM_MAT4 ? 16 : 4);

				if (u.handler < 0)
					u.type = RoxShader::UNIFORM_NOT_FOUND;
			}
			shdr.uniform_cache.resize(cache_size);

			setShader(-1);
		}

		return idx;
	}

	bool RoxRenderOpengl::getProgramBinaryShader(int idx, RoxCompiledShader& cmp_shdr)
	{
		if (idx < 0 || idx >= shaders.getCount())
		{
			log() << "ERROR: Invalid shader index\n";
			return false;
		}

		setShader(idx);

		if (applied_state.shader < 0)
		{
			log() << "ERROR: No active shader program to save binary.\n";
			return false;
		}

		ShaderObj& current_shader = shaders.get(applied_state.shader);

		GLint linked;
		glGetProgramiv(current_shader.program, GL_LINK_STATUS, &linked);
		if (!linked)
		{
			log() << "ERROR: Shader program not linked.\n";
			return false;
		}

		GLint binary_length;
		glGetProgramiv(current_shader.program, GL_PROGRAM_BINARY_LENGTH, &binary_length);
		if (binary_length <= 0)
		{
			log() << "ERROR: No binary data available.\n";
			return false;
		}

		size_t total_size = binary_length + sizeof(GLenum);
		cmp_shdr = RoxCompiledShader(total_size);

		GLenum binary_format;
		glGetProgramBinary(
			current_shader.program, 
			binary_length, 
			nullptr, 
			&binary_format, 
			static_cast<char*>(cmp_shdr.getData()) + sizeof(GLenum)
		);
		if (!cmp_shdr.getData())
		{
			log() << "ERROR::PROGRAM::SHADER::BINARY::FAILED\n";
			return false;
		}

		// Store the binary format at the start of the binary data
		//memcpy(current_shader.binaryShader.getData(), &binary_format, sizeof(GLenum));
		*reinterpret_cast<GLenum*>(cmp_shdr.getData()) = binary_format;

		return true;
	}

	namespace
	{
		RoxMath::Matrix4 modelview, projection;
	}

	void RoxRenderOpengl::setCamera(const RoxMath::Matrix4& mv, const RoxMath::Matrix4& p)
	{
		modelview = mv;
		projection = p;
	}

	void RoxRenderOpengl::applyState(const State& s)
	{
		State& a = applied_state;

		apply_viewport_state(s);

		if (s.blend != a.blend || ignore_cache)
		{
			if (s.blend)
				glEnable(GL_BLEND);
			else
				glDisable(GL_BLEND);
			a.blend = s.blend;
		}

		if (s.blend_src != a.blend_src || s.blend_dst != a.blend_dst || ignore_cache)
		{
			glBlendFuncSeparate(gl_blend_mode(s.blend_src), gl_blend_mode(s.blend_dst),GL_ONE,GL_ONE);
			a.blend_src = s.blend_src, a.blend_dst = s.blend_dst;
		}

		if (s.cull_face != a.cull_face || ignore_cache)
		{
			if (s.cull_face)
				glEnable(GL_CULL_FACE);
			else
				glDisable(GL_CULL_FACE);
			a.cull_face = s.cull_face;
		}

		if (s.cull_order != a.cull_order || ignore_cache)
		{
			if (s.cull_order == CullFace::CW)
				glFrontFace(GL_CW);
			else
				glFrontFace(GL_CCW);
			a.cull_order = s.cull_order;
		}

		if (s.depth_test != a.depth_test || ignore_cache)
		{
			if (s.depth_test)
				glEnable(GL_DEPTH_TEST);
			else
				glDisable(GL_DEPTH_TEST);
			a.depth_test = s.depth_test;
		}

		if (s.depth_comparison != a.depth_comparison || ignore_cache)
		{
			switch (s.depth_comparison)
			{
			case DepthTest::NEVER: glDepthFunc(GL_NEVER);
				break;
			case DepthTest::LESS: glDepthFunc(GL_LESS);
				break;
			case DepthTest::EQUAL: glDepthFunc(GL_EQUAL);
				break;
			case DepthTest::GREATER: glDepthFunc(GL_GREATER);
				break;
			case DepthTest::NOT_LESS: glDepthFunc(GL_GEQUAL);
				break;
			case DepthTest::NOT_EQUAL: glDepthFunc(GL_NOTEQUAL);
				break;
			case DepthTest::NOT_GREATER: glDepthFunc(GL_LEQUAL);
				break;
			case DepthTest::ALLWAYS: glDepthFunc(GL_ALWAYS);
				break;
			}
			a.depth_comparison = s.depth_comparison;
		}

		if (s.zwrite != a.zwrite || ignore_cache)
		{
			glDepthMask(s.zwrite);
			a.zwrite = s.zwrite;
		}

		if (s.color_write != a.color_write || ignore_cache)
		{
			glColorMask(s.color_write, s.color_write, s.color_write, s.color_write);
			a.color_write = s.color_write;
		}

		for (int i = 0; i < s.max_layers; ++i)
		{
			if (s.textures[i] != a.textures[i])
				set_texture(s.textures[i], i);
		}

		ignore_cache = false;
	}

	template <bool transform_feedback>
	void draw_(const RoxRender::IRoxRenderAPI::State& s)
	{
		if (s.vertex_buffer < 0 || s.shader < 0)
			return;

		setShader(s.shader);

		ShaderObj& shdr = shaders.get(s.shader);
		if (shdr.mat_mvp >= 0)
		{
			const RoxMath::Matrix4 mvp = modelview * projection;
			glUniformMatrix4fv(shdr.mat_mvp, 1,GL_FALSE, mvp[0]);
		}
		if (shdr.mat_mv >= 0)
			glUniformMatrix4fv(shdr.mat_mv, 1,GL_FALSE, modelview[0]);
		if (shdr.mat_p >= 0)
			glUniformMatrix4fv(shdr.mat_p, 1,GL_FALSE, projection[0]);

		VertexBuffer& v = vert_bufs.get(s.vertex_buffer);

		if (s.vertex_buffer != applied_state.vertex_buffer)
		{
			if (v.vertex_array_object > 0)
			{
				glBindVertexArray(v.vertex_array_object);
			}
			else
			{
				glGenVertexArrays(1, &v.vertex_array_object);
				glBindVertexArray(v.vertex_array_object);

				applied_layout = RoxVBO::Layout();
				v.active_vao_ibuf = 0;
				glBindBuffer(GL_ARRAY_BUFFER, v.id);

				glEnableVertexAttribArray(VERTEX_ATTRIBUTE);
				glVertexAttribPointer(VERTEX_ATTRIBUTE, v.layout.pos.dimension, getGLElementType(v.layout.pos.type),
				                      true,
				                      v.stride, (void*)(ptrdiff_t)(v.layout.pos.offset));
				for (unsigned int i = 0; i < RoxVBO::max_tex_coord; ++i)
				{
					const RoxVBO::Layout::Attribute& tex_coord = v.layout.tex_coord[i];
					if (tex_coord.dimension > 0)
					{
						//if(vobj.vertex_stride!=active_attributes.vertex_stride || !tc.compare(active_attributes.tcs[i]))
						{
							if (!applied_layout.tex_coord[i].dimension)
								glEnableVertexAttribArray(TC0_ATTRIBUTE + i);
							glVertexAttribPointer(TC0_ATTRIBUTE + i, tex_coord.dimension, getGLElementType(tex_coord.type), true,
							                      v.stride, (void*)(ptrdiff_t)(tex_coord.offset));
						}
					}
					else if (applied_layout.tex_coord[i].dimension > 0)
						glDisableVertexAttribArray(TC0_ATTRIBUTE + i);
				}

				if (v.layout.normal.dimension > 0)
				{
					//if(vobj.vertex_stride!=active_attributes.vertex_stride || !vobj.normals.compare(active_attributes.normals))
					{
						if (!applied_layout.normal.dimension)
							glEnableVertexAttribArray(NORMAL_ATTRIBUTE);
						glVertexAttribPointer(NORMAL_ATTRIBUTE, 3, getGLElementType(v.layout.normal.type), true,
						                      v.stride, (void*)(ptrdiff_t)(v.layout.normal.offset));
					}
				}
				else if (applied_layout.normal.dimension > 0)
					glDisableVertexAttribArray(NORMAL_ATTRIBUTE);

				if (v.layout.color.dimension > 0)
				{
					//if(vobj.vertex_stride!=active_attributes.vertex_stride || !vobj.colors.compare(active_attributes.colors))
					{
						if (!applied_layout.color.dimension)
							glEnableVertexAttribArray(COLOR_ATTRIBUTE);
						glVertexAttribPointer(COLOR_ATTRIBUTE, v.layout.color.dimension,
						                      getGLElementType(v.layout.color.type), true,
						                      v.stride, (void*)(ptrdiff_t)(v.layout.color.offset));
					}
				}
				else if (applied_layout.color.dimension > 0)
					glDisableVertexAttribArray(COLOR_ATTRIBUTE);
				applied_layout = v.layout;
			}
			applied_state.vertex_buffer = s.vertex_buffer;
		}

		const int gl_elem = getGLElementType(s.primitive);
		if (s.index_buffer >= 0)
		{
			IndexBuffer& i = ind_bufs.get(s.index_buffer);

			if (i.id != v.active_vao_ibuf)
			{
				glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, i.id);
				v.active_vao_ibuf = i.id;
			}

			const unsigned int gl_elem_type = (i.type == RoxVBO::INDEX_4D ? GL_UNSIGNED_INT : GL_UNSIGNED_SHORT);
#ifdef USE_INSTANCING
        if(s.instances_count>1 && glDrawElementsInstancedARB)
            glDrawElementsInstancedARB(gl_elem,s.index_count,gl_elem_type,(void*)(ptrdiff_t)(s.index_offset*i.type),s.instances_count);
        else
#endif
			if (transform_feedback)
			{
				glBeginTransformFeedback(getGLElementType(s.primitive));
				glDrawElements(gl_elem, s.index_count, gl_elem_type, (void*)(ptrdiff_t)(s.index_offset * i.type));
				glEndTransformFeedback();
			}
			else
				glDrawElements(gl_elem, s.index_count, gl_elem_type, (void*)(ptrdiff_t)(s.index_offset * i.type));
		}
		else
		{
#ifdef USE_INSTANCING
        if(s.instances_count>1 && glDrawArraysInstancedARB)
            glDrawArraysInstancedARB(gl_elem,s.index_offset,s.index_count,s.instances_count);
        else
#endif
			if (transform_feedback)
			{
				glBeginTransformFeedback(getGLElementType(s.primitive));
				glDrawArrays(gl_elem, s.index_offset, s.index_count);
				glEndTransformFeedback();
			}
			else
				glDrawArrays(gl_elem, s.index_offset, s.index_count);
		}
	}

	void RoxRenderOpengl::draw(const State& s)
	{
		applyState(s);
		draw_<false>(s);
	}

	void RoxRenderOpengl::transformFeedback(const TfState& s)
	{
		if (!isTransformFeedbackSupported())
			return;

		//glEnable(GL_RASTERIZER_DISCARD);
		const VertexBuffer& dst = vert_bufs.get(s.vertex_buffer_out);
		if (!s.out_offset)
			glBindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 0, dst.id);
		else
			glBindBufferRange(GL_TRANSFORM_FEEDBACK_BUFFER, 0, dst.id, s.out_offset * dst.stride,
			                    s.index_count * dst.stride);
		active_transform_feedback = s.vertex_buffer_out;

		State ss = applied_state;
		(RenderState&)ss = s;
		for (int i = 0; i < s.max_layers; ++i)
		{
			if (s.textures[i] != applied_state.textures[i])
				set_texture(s.textures[i], i);
		}
		draw_<true>(ss);
		//glDisable(GL_RASTERIZER_DISCARD);
	}

	bool RoxRenderOpengl::isTransformFeedbackSupported()
	{
#ifdef NO_EXTENSIONS_INIT
    return true;
#else
		return glBeginTransformFeedback != 0;
#endif
	}

	unsigned int RoxRenderOpengl::getGlTextureId(int idx)
	{
		return textures.get(idx).tex_id;
	}

	void RoxRenderOpengl::rglBindTexture(uint gl_type, uint gl_idx, uint layer)
	{
		set_texture(-1, layer);
		glSelectMultitexLayer((int)layer);
		glBindTexture(gl_type, gl_idx);
	}

	void RoxRenderOpengl::rglBindTexture2d(uint gl_tex, uint layer)
	{
		rglBindTexture(GL_TEXTURE_2D, gl_tex, layer);
	}

	void RoxRenderOpengl::logErrors(const char* place)
	{
		for (int i = glGetError(); i != GL_NO_ERROR; i = glGetError())
		{
			log() << "gl error: ";
			switch (i)
			{
			case GL_INVALID_ENUM: log() << "invalid enum";
				break;
			case GL_INVALID_VALUE: log() << "invalid value";
				break;
			case GL_INVALID_OPERATION: log() << "invalid operation";
				break;
#if !defined OPENGL3
			case GL_STACK_OVERFLOW: log() << "stack overflow";
				break;
			case GL_STACK_UNDERFLOW: log() << "stack underflow";
				break;
#endif
			case GL_OUT_OF_MEMORY: log() << "out of memory";
				break;

			default: log() << "unknown";
				break;
			}

			log() << " (" << i << ")";
			if (place)
				log() << " at " << place << "\n";
			else
				log() << "\n";
		}
	}

	namespace
	{
#ifdef GL_DEBUG_OUTPUT
#ifdef _WIN32
    void GLAPIENTRY debug_log(GLenum source,GLenum type,GLuint id,GLenum severity,GLsizei length,const GLchar *message,const void* userParam)
#else
    void debug_log(GLenum source,GLenum type,GLuint id,GLenum severity,GLsizei length,const GLchar* message,const void* userParam)
#endif
    {
        const char* source_str;
        const char* type_str;
        const char* severity_str;

        switch (source)
        {
            case GL_DEBUG_SOURCE_API:               source_str = "api"; break;
            case GL_DEBUG_SOURCE_WINDOW_SYSTEM:     source_str = "window system"; break;
            case GL_DEBUG_SOURCE_SHADER_COMPILER:   source_str = "RoxShader compiler"; break;
            case GL_DEBUG_SOURCE_THIRD_PARTY:       source_str = "third party"; break;
            case GL_DEBUG_SOURCE_APPLICATION:       source_str = "application"; break;
            case GL_DEBUG_SOURCE_OTHER:             source_str = "other"; break;
            default:                                source_str = "unknown"; break;
        }

        switch (type)
        {
            case GL_DEBUG_TYPE_ERROR:               type_str = "error";  break;
            case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR: type_str = "deprecated behaviour"; break;
            case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:  type_str = "undefined behaviour"; break;
            case GL_DEBUG_TYPE_PORTABILITY:         type_str = "portability"; break;
            case GL_DEBUG_TYPE_PERFORMANCE:         type_str = "performance"; break;
            case GL_DEBUG_TYPE_MARKER:              type_str = "marker"; break;
            case GL_DEBUG_TYPE_OTHER:               type_str = "other"; break;
            default:                                type_str = "unknown"; break;
        }

        switch (severity)
        {
            case GL_DEBUG_SEVERITY_HIGH:         severity_str = "high"; break;
            case GL_DEBUG_SEVERITY_MEDIUM:       severity_str = "medium"; break;
            case GL_DEBUG_SEVERITY_LOW:          severity_str = "low"; break;
            case GL_DEBUG_SEVERITY_NOTIFICATION: severity_str = "notification"; break;
            default:                             severity_str = "unknown"; break;
        }

        RoxLogger::log("gl log [%s] %s %s %d: %s\n", severity_str, source_str, type_str, id, message);
        if(severity!=GL_DEBUG_SEVERITY_NOTIFICATION)
            severity_str=severity_str;
    }
#endif
		bool log_set = false;
	}

	void RoxRenderOpengl::enableDebug(bool synchronous)
	{
		if (log_set)
			return;
		log_set = true;

#ifdef GL_DEBUG_OUTPUT
#ifndef NO_EXTENSIONS_INIT
    
    if(!glDebugMessageCallback)
    {
        RoxLogger::log("unable to set opengl log\n");
        return;
    }
#endif

    glEnable(GL_DEBUG_OUTPUT);
    if(synchronous)
        glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
    glDebugMessageCallback(debug_log,NULL);
#endif
	}

	RoxRenderOpengl& RoxRenderOpengl::get()
	{
		static RoxRenderOpengl* api = new RoxRenderOpengl();
		return *api;
	}
}
