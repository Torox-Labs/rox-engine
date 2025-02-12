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

#include "RoxShaderCodeParser.h"
#include "RoxLogger/RoxLogger.h"
#include <cstdlib>
#include <cstring>
#include <cstdio>
#include <algorithm>

namespace RoxRender
{
	template <typename t>
	static void pushUniqueToVector(std::vector<t>& v, const t& e)
	{
		for (size_t i = 0; i < v.size(); ++i)
		{
			if (v[i].name == e.name)
			{
				v[i] = e;
				return;
			}
		}

		v.push_back(e);
	}

	inline int typeRegsize(RoxShaderCodeParser::VARIABLE_TYPE type)
	{
		if (type == RoxShaderCodeParser::TYPE_MATRIX2) return 2;
		if (type == RoxShaderCodeParser::TYPE_MATRIX3) return 3;
		if (type == RoxShaderCodeParser::TYPE_MATRIX4) return 4;
		return 1;
	}

	static bool isNameChar(char c) { return isalnum(c) || c == '_'; }

	bool RoxShaderCodeParser::convertToHlsl()
	{
		m_uniforms.clear();
		m_attributes.clear();

		std::string prefix = "#define DIRECTX11 1\n";

		parsePredefinedUniforms(m_replace_str.c_str(), true);
		if (!m_uniforms.empty())
		{
			std::sort(m_uniforms.begin(), m_uniforms.end());

			prefix.append("cbuffer " + m_replace_str + "constant_buffer:register(b0){");
			for (size_t i = 0; i < m_uniforms.size(); ++i)
			{
				std::string type = m_uniforms[i].type == TYPE_FLOAT ? "float" : "matrix";
				prefix.append(type + " " + m_uniforms[i].name + ";");
			}
			prefix.append("}\n");
		}

		const size_t predefined_count = m_uniforms.size();

		if (!parseUniforms(true))
		{
			m_error.append("unable to parse uniforms\n");
			return false;
		}

		if (!parseVarying(true))
		{
			m_error.append("unable to parse varying\n");
			return false;
		}

		std::sort(m_varying.begin(), m_varying.end());

		const std::string replace_constructor = m_replace_str + "cast_float";
		if (replaceVecFromFloat(replace_constructor.c_str()))
		{
			prefix.append("float2 " + replace_constructor + "2(float a){return float2(a,a);} ");
			prefix.append("float2 " + replace_constructor + "2(float2 a){return a;}\n");
			prefix.append("float3 " + replace_constructor + "3(float a){return float3(a,a,a);} ");
			prefix.append("float3 " + replace_constructor + "3(float3 a){return a;}\n");
			prefix.append("float4 " + replace_constructor + "4(float a){return float4(a,a,a,a);} ");
			prefix.append("float4 " + replace_constructor + "4(float4 a){return a;}\n");
		}

		replaceHlslMul("mul");
		replaceHlslTypes();

		replaceVariable("mix", "lerp");
		replaceVariable("fract", "frac");
		replaceVariable("inversesqrt", "rsqrt");
		if (replaceVariable("pow", (m_replace_str + "pow").c_str()))
			prefix.append("#define " + m_replace_str + "pow(f,e) pow(abs(f),e)\n");

		int samplers_count = 0;
		for (size_t i = predefined_count; i < m_uniforms.size(); ++i)
		{
			const Variable& v = m_uniforms[i];
			if (v.type != TYPE_SAMPLER2D && v.type != TYPE_SAMPLER_CUBE)
				continue;

			const char* types[] = {"Texture2D", "TextureCube"};

			char buf[512];
			// TODO: CONVERTED FROM _nya_ to _rox_
			printf(buf, "%s %s: register(t%d); SamplerState %s_nya_st: register(s%d);\n",
			       types[v.type - TYPE_SAMPLER2D], v.name.c_str(), samplers_count, v.name.c_str(), samplers_count);
			prefix.append(buf);
			++samplers_count;
		}

		if (samplers_count > 0)
		{
			if (findVariable("texture2D")) prefix.append(
				"#define texture2D(a,b) a.Sample(a##" + m_replace_str + "st,(b))\n");
			if (findVariable("textureCube")) prefix.append(
				"#define textureCube(a,b) a.Sample(a##" + m_replace_str + "st,(b))\n");
			if (findVariable("texture2DProj"))
			{
				prefix.append("float2 " + m_replace_str + "tc_proj(float3 tc){return tc.xy/tc.z;}\n");
				prefix.append("float2 " + m_replace_str + "tc_proj(float4 tc){return tc.xy/tc.w;}\n");
				prefix.append(
					"#define texture2DProj(a,b) a.Sample(a##" + m_replace_str + "st," + m_replace_str +
					"tc_proj(b))\n");
			}
		}

		const char *gl_vs_out = "gl_Position", *gl_ps_out = "gl_FragColor";
		const char* type_names[] = {"float", "float2", "float3", "float4", "float2x2", "float3x3", "float4x4"};

		std::string vs_pos_out = m_replace_str + std::string(gl_vs_out + 3);
		prefix.append("struct " + m_replace_str + "vsout{float4 " + vs_pos_out + ":SV_POSITION;");
		for (int i = 0, idx = 0; i < (int)m_varying.size(); ++i)
		{
			const Variable& v = m_varying[i];
			if (v.type == TYPE_INVALID)
				return false;

			if (v.type - 1 >= sizeof(type_names) / sizeof(type_names[0]))
				continue;

			char buf[255];
			printf(buf, "%s %s:TEXCOORD%d;", type_names[v.type], m_varying[i].name.c_str(), idx);
			prefix.append(buf);
			idx += typeRegsize(v.type);
		}
		prefix.append("};\n");

		const std::string input_var = m_replace_str + "in";
		const std::string ps_out_var = m_replace_str + std::string(gl_ps_out + 3); //strlen("gl_")
		const bool is_fragment = replaceVariable(gl_ps_out, ps_out_var.c_str());
		if (is_fragment)
		{
			prefix.append("static float4 " + ps_out_var + ";\n");

			const std::string main = std::string("void ") + m_replace_str + "main()";
			if (!replaceMainFunctionHeader(main.c_str()))
			{
				m_error.append("main function not found\n");
				return false;
			}

			std::string in_var_assign;
			for (int i = 0; i < (int)m_varying.size(); ++i)
			{
				const Variable& v = m_varying[i];
				if (v.type == TYPE_INVALID)
				{
					m_error.append("invalid variable \'" + v.name + "\' type\n");
					return false;
				}

				if (v.type - 1 >= sizeof(type_names) / sizeof(type_names[0]))
					continue;

				prefix.append("static " + std::string(type_names[v.type]) + " " + m_varying[i].name + ";");
				in_var_assign.append(m_varying[i].name + "=" + input_var + "." + m_varying[i].name + ";");
			}
			prefix.append("\n");

			m_code.append("\nfloat4 main(" + m_replace_str + "vsout " + input_var + "):SV_TARGET{" + in_var_assign +
				m_replace_str + "main();return " + ps_out_var + ";}\n");
		}
		else
		{
			if (!parseAttributes(m_replace_str.c_str(), (input_var + ".").c_str()))
			{
				m_error.append("unable to parse attributes\n");
				return false;
			}

			if (!m_attributes.empty())
			{
				prefix.append("struct " + m_replace_str + "vsin{");
				for (int i = 0, idx = 0; i < (int)m_attributes.size(); ++i)
				{
					Variable& a = m_attributes[i];
					a.name = a.name.substr(m_replace_str.size());
					if (a.name == "Vertex")
						prefix.append("float4 Vertex:POSITION;");
					else if (a.name == "Normal")
						prefix.append("float3 Normal:NORMAL;");
					else if (a.name == "Color")
						prefix.append("float4 Color:COLOR;");
					else
					{
						char buf[255];
						printf(buf, "float4 %s:TEXCOORD%d;", a.name.c_str(), idx);
						prefix.append(buf);
						idx += typeRegsize(a.type);
					}
				}

				std::string instance_var = m_replace_str + "InstanceID";
				if (replaceVariable("gl_InstanceID", (input_var + "." + instance_var).c_str()))
					prefix.append("uint " + instance_var + ":SV_InstanceID;");

				prefix.append("};\n");
			}

			const std::string out_var = m_replace_str + "out";
			prefix.append("static " + m_replace_str + "vsin " + input_var + ";\n");

			const std::string main = std::string("void ") + m_replace_str + "main()";
			replaceMainFunctionHeader(main.c_str());
			const std::string vs_out_var = out_var + "." + m_replace_str + std::string(gl_vs_out + 3); //strlen("gl_")

			replaceVariable(gl_vs_out, vs_pos_out.c_str());

			std::string out_var_assign;
			prefix.append("static float4 " + vs_pos_out + ";");
			out_var_assign.append(out_var + "." + vs_pos_out + "=" + vs_pos_out + ";");
			for (int i = 0; i < (int)m_varying.size(); ++i)
			{
				const Variable& v = m_varying[i];
				if (v.type == TYPE_INVALID)
					return false;

				if (v.type - 1 >= sizeof(type_names) / sizeof(type_names[0]))
					continue;

				prefix.append("static " + std::string(type_names[v.type]) + " " + m_varying[i].name + ";");
				out_var_assign.append(out_var + "." + m_varying[i].name + "=" + m_varying[i].name + ";");
			}
			prefix.append("\n");

			if (!m_flip_y_uniform.empty())
				out_var_assign.append(out_var + "." + vs_pos_out + ".y*=" + m_flip_y_uniform + ";");

			m_code.append(
				"\n" + m_replace_str + "vsout main(" + m_replace_str + "vsin " + input_var + "_){" + input_var + "=" +
				input_var + "_;" +
				m_replace_str + "main();" + m_replace_str + "vsout " + out_var + ";" + out_var_assign + "return " +
				out_var + ";}\n");
		}

		if (m_uniforms.size() > predefined_count)
		{
			prefix.append("cbuffer " + m_replace_str + "uniforms_buffer:register(b"),
				prefix.append(is_fragment ? "2" : "1"), prefix.append("){");

			for (size_t i = predefined_count; i < m_uniforms.size(); ++i)
			{
				const Variable& v = m_uniforms[i];
				if (v.type == TYPE_INVALID)
					return false;

				if (v.type - 1 >= sizeof(type_names) / sizeof(type_names[0]))
					continue;

				prefix.append(type_names[v.type]), prefix.append(" " + v.name);
				if (v.array_size > 1)
				{
					char buf[255];
					printf(buf, "[%d];", v.array_size);
					prefix.append(buf);
				}
				else
					prefix.append(";");
			}
			prefix.append("}\n");
		}

		m_code.insert(0, prefix);
		m_varying.clear();
		return true;
	}

	inline bool hasArgs(std::string& code, size_t pos)
	{
		for (size_t j = pos; j < code.size(); ++j)
		{
			if (code[j] == ')')
				break;

			if (code[j] > ' ')
			{
				if (strncmp(&code[j], "void", 4) == 0)
					memset(&code[j], ' ', 4);
				else
					return true;
			}
		}

		return false;
	}

	bool RoxShaderCodeParser::convertToMetal()
	{
		m_uniforms.clear();
		m_attributes.clear();

		parsePredefinedUniforms(m_replace_str.c_str(), true);
		if (!parseUniforms(true))
		{
			m_error.append("unable to parse uniforms\n");
			return false;
		}

		if (!parseVarying(true))
		{
			m_error.append("unable to parse varying\n");
			return false;
		}

		std::string prefix = "#include <metal_stdlib>\n#include <simd/simd.h>\nusing namespace metal;\n";
		const char *gl_vs_out = "gl_Position", *gl_ps_out = "gl_FragColor";
		const char* type_names[] = {"float", "float2", "float3", "float4", "float2x2", "float3x3", "float4x4"};

		const std::string uniforms_type = m_replace_str + "uniforms";
		const std::string uniforms_name = m_replace_str + "u";
		if (!m_uniforms.empty())
		{
			prefix.append("struct " + uniforms_type + '{');
			for (size_t i = 0; i < m_uniforms.size(); ++i)
			{
				const Variable& v = m_uniforms[i];
				if (v.type == TYPE_INVALID)
					return false;

				if (v.type >= sizeof(type_names) / sizeof(type_names[0]))
					continue;

				prefix.append(type_names[v.type]), prefix.append(" " + v.name);
				if (v.array_size > 1)
				{
					char buf[255];
					printf(buf, "[%d];", v.array_size);
					prefix.append(buf);
				}
				else
					prefix.append(";");

				replaceVariable(v.name.c_str(), (uniforms_name + '.' + v.name).c_str());
			}
			prefix.append("};\n");
		}

		const std::string vertex_type = m_replace_str + "vertex";
		const bool is_fragment = replaceVariable(gl_ps_out, "out");

		std::string vs_pos_out = m_replace_str + std::string(gl_vs_out + 3);
		const std::string varying_type = m_replace_str + "varying";
		if (!m_varying.empty() || !is_fragment)
		{
			prefix.append("struct " + varying_type + "{");
			if (!is_fragment)
				prefix.append("float4 " + vs_pos_out + "[[position]];");

			for (int i = 0; i < (int)m_varying.size(); ++i)
			{
				const Variable& v = m_varying[i];
				if (v.type == TYPE_INVALID)
				{
					m_error.append("invalid Variable \'" + v.name + "\' type\n");
					return false;
				}

				if (v.type - 1 >= sizeof(type_names) / sizeof(type_names[0]))
					continue;

				prefix.append(std::string(type_names[v.type]) + ' ' + m_varying[i].name + ';');
			}
			prefix.append("};\n");
		}

		struct
		{
			std::string decl;
			std::string call;
			std::string main;

			void add(std::string type, std::string name, std::string semantics)
			{
				if (!decl.empty())
				{
					decl.push_back(',');
					call.push_back(',');
					main.push_back(',');
				}

				decl.append(type + ' ' + name);
				call.append(name);
				main.append(type + ' ' + name + semantics);
			}
		} args;

		int samplers_count = 0;
		for (size_t i = 0; i < m_uniforms.size(); ++i)
		{
			const Variable& v = m_uniforms[i];
			if (v.type != TYPE_SAMPLER2D && v.type != TYPE_SAMPLER_CUBE)
				continue;

			const char* types[] = {"texture2d<float>", "texturecube<float>"};

			char buf[64];
			printf(buf, "[[texture(%d)]]", samplers_count);
			args.add(types[v.type - TYPE_SAMPLER2D], v.name, buf);
			printf(buf, "[[sampler(%d)]]", samplers_count);
			args.add("sampler", v.name + m_replace_str + "st", buf);

			++samplers_count;
		}

		if (samplers_count > 0)
		{
			if (findVariable("texture2D")) prefix.append(
				"#define texture2D(a,b) a.sample(a##" + m_replace_str + "st,(b))\n");
			if (findVariable("textureCube")) prefix.append(
				"#define textureCube(a,b) a.sample(a##" + m_replace_str + "st,(b))\n");
			if (findVariable("texture2DProj"))
			{
				prefix.append("float2 " + m_replace_str + "tc_proj(float3 tc){return tc.xy/tc.z;}\n");
				prefix.append("float2 " + m_replace_str + "tc_proj(float4 tc){return tc.xy/tc.w;}\n");
				prefix.append(
					"#define texture2DProj(a,b) a.sample(a##" + m_replace_str + "st," + m_replace_str +
					"tc_proj(b))\n");
			}
		}

		std::string out_decl;
		std::string main_type;
		if (is_fragment)
		{
			main_type = "fragment float4";
			out_decl = "\nfloat4 out;\n";

			if (!m_varying.empty())
			{
				args.add(varying_type, "in", "[[stage_in]]");
				for (int i = 0; i < (int)m_varying.size(); ++i)
					replaceVariable(m_varying[i].name.c_str(), ("in." + m_varying[i].name).c_str());
			}

			if (!m_uniforms.empty())
				args.add("constant " + uniforms_type + '&', uniforms_name, "[[buffer(0)]]");
		}
		else
		{
			main_type = "vertex " + varying_type;
			out_decl = '\n' + varying_type + " out;\n";

			args.add(vertex_type, "in", "[[stage_in]]");

			if (!parseAttributes(m_replace_str.c_str(), "in."))
			{
				m_error.append("unable to parse attributes\n");
				return false;
			}

			prefix.append("struct " + vertex_type + "{ ");
			for (int i = 0; i < (int)m_attributes.size(); ++i)
			{
				Variable& a = m_attributes[i];
				a.name = a.name.substr(m_replace_str.size());
				if (a.name == "Vertex")
					prefix.append("float4 Vertex[[attribute(0)]];");
				else if (a.name == "Normal")
					prefix.append("float3 Normal[[attribute(1)]];");
				else if (a.name == "Color")
					prefix.append("float4 Color[[attribute(2)]];");
				else
				{
					char buf[255];
					printf(buf, "float4 %s[[attribute(%d)]];", a.name.c_str(), a.idx + 3);
					prefix.append(buf);
				}
			}
			prefix.append("};\n");

			const std::string vertexID = m_replace_str + "VertexID";
			if (replaceVariable("gl_VertexID", vertexID.c_str()))
				args.add("uint", vertexID, "[[vertex_id]]");

			const std::string instanceID = m_replace_str + "InstanceID";
			if (replaceVariable("gl_InstanceID", instanceID.c_str()))
				args.add("uint", instanceID, "[[instance_id]]");

			for (int i = 0; i < (int)m_varying.size(); ++i)
				replaceVariable(m_varying[i].name.c_str(), ("out." + m_varying[i].name).c_str());
			replaceVariable(gl_vs_out, ("out." + vs_pos_out).c_str());

			if (!m_uniforms.empty())
				args.add("constant " + uniforms_type + '&', uniforms_name, "[[buffer(1)]]");
		}

		replaceHlslTypes();
		replaceVariable("discard", "discard_fragment()");

		std::vector<std::string> functions;

		int parentness = 0;
		size_t func_start = 0;
		bool is_main = false;
		for (size_t i = 0; i < m_code.size(); ++i)
		{
			if (m_code[i] == '}')
			{
				if (--parentness == 0)
				{
					if (is_main)
					{
						const std::string return_out = "\nreturn out;\n";
						m_code.insert(i, return_out);
						i += return_out.size();
					}

					for (int j = 0; j < (int)functions.size(); ++j)
					{
						for (size_t pos = m_code.find(functions[j].c_str(), func_start); pos < i; pos = m_code.find(
							     functions[j].c_str(), pos + 1))
						{
							if (isNameChar(m_code[pos - 1]) || isNameChar(m_code[pos + functions[j].length()]))
								continue;

							size_t arg_start = m_code.find('(', pos);
							if (arg_start > i)
								continue;

							const bool add_comma = hasArgs(m_code, ++arg_start);
							m_code.insert(arg_start, args.call);
							i += args.call.size();
							pos = arg_start + args.call.size();
							if (add_comma)
							{
								m_code.insert(pos, ",");
								++i;
							}
						}
					}
				}
				continue;
			}

			if (m_code[i] == '{')
			{
				if (++parentness > 1)
					continue;

				size_t arg_start = m_code.rfind('(', i);
				if (arg_start == std::string::npos)
				{
					m_error = "invalid function";
					return false;
				}

				size_t name_end = arg_start;
				while (name_end > 1) if (m_code[name_end] > ' ') break;
				else --name_end;
				size_t name_start = name_end - 1;
				while (name_start > 0) if (m_code[name_start] <= ' ') break;
				else --name_start;
				++name_start;
				const std::string name = m_code.substr(name_start, name_end - name_start);

				const bool add_comma = hasArgs(m_code, ++arg_start);
				is_main = (name == "main");
				if (is_main)
				{
					m_code.insert(i + 1, out_decl);
					i += out_decl.size();
					m_code.insert(arg_start, args.main);
					i += args.main.size();
					arg_start += args.main.size();
					m_code.insert(name_start, m_replace_str);
					i += m_replace_str.size();
					arg_start += m_replace_str.size();
					size_t type_pos = m_code.rfind("void", name_start);
					if (type_pos == std::string::npos)
					{
						m_error = "invalid main function type";
						return false;
					}
					m_code.replace(type_pos, 4, main_type);
					i += main_type.size() - 4;
					arg_start += main_type.size() - 4;
				}
				else
				{
					m_code.insert(arg_start, args.decl);
					i += args.decl.size();
					arg_start += args.decl.size();
					const std::string inline_str = "inline ";
					m_code.insert(name_start, inline_str);
					i += inline_str.size();
					arg_start += inline_str.size();
					functions.push_back(name);
				}

				if (add_comma)
				{
					m_code.insert(arg_start, ",");
					++i;
				}
				func_start = i + 1;
			}
		}

		if (parentness != 0)
			return false;

		m_code.insert(0, prefix);
		m_varying.clear();
		return true;
	}

	bool RoxShaderCodeParser::convertToGlsl()
	{
		if (replaceVariable("gl_InstanceID", "gl_InstanceIDARB"))
			m_code.insert(0, "#extension GL_ARB_draw_instanced:enable\n");
		if (findVariable("gl_VertexID"))
			m_code.insert(0, "#extension GL_EXT_gpu_shader4:require\n");
		return true;
	}

	bool RoxShaderCodeParser::convertToGlsl3()
	{
		m_uniforms.clear();
		m_attributes.clear();

		if (!parsePredefinedUniforms(m_replace_str.c_str(), true))
		{
			m_error.append("unable to parse predefined uniforms\n");
			return false;
		}

		if (!parseAttributes(m_replace_str.c_str(), m_replace_str.c_str()))
		{
			m_error.append("unable to parse predefined attributes\n");
			return false;
		}

		const bool require_gl4 = findVariable("textureQueryLod") || m_code.find("textureGather") != std::string::npos;

		std::string prefix = require_gl4 ? "#version 400 core\n" : "#version 330 core\n";

		for (int i = 0; i < (int)m_uniforms.size(); ++i)
			prefix.append("uniform mat4 " + m_uniforms[i].name + ";\n");

		parseUniforms(false);

		for (int i = 0; i < (int)m_attributes.size(); ++i)
		{
			prefix.append("in ");
			prefix.append(m_attributes[i].type == TYPE_VECTOR3 ? "vec3 " : "vec4 ");
			prefix.append(m_attributes[i].name + ";\n");
		}

		//prefix.append("precision highp float;\n");

		replaceVariable("texture2D", "texture");
		replaceVariable("texture2DProj", "textureProj");
		replaceVariable("textureCube", "texture");

		const char* gl_ps_out = "gl_FragColor";
		std::string ps_out_var = m_replace_str + std::string(gl_ps_out + 3);
		const bool is_fragment = replaceVariable(gl_ps_out, ps_out_var.c_str());
		if (is_fragment)
		{
			prefix.append("layout(location=0) out vec4 " + ps_out_var + ";\n");
			replaceVariable("varying", "in");
		}
		else
		{
			//replaceVariable("attribute","in");
			replaceVariable("varying", "out");
		}

		m_code.insert(0, prefix);
		return true;
	}

	int RoxShaderCodeParser::getUniformsCount()
	{
		if (m_uniforms.empty())
		{
			parsePredefinedUniforms(m_replace_str.c_str(), false);
			parseUniforms(false);
		}

		return (int)m_uniforms.size();
	}

	RoxShaderCodeParser::Variable RoxShaderCodeParser::getUniform(int idx) const
	{
		if (idx < 0 || idx >= (int)m_uniforms.size())
			return RoxShaderCodeParser::Variable();

		return m_uniforms[idx];
	}

	int RoxShaderCodeParser::getAttributesCount()
	{
		if (m_attributes.empty())
			parseAttributes(m_replace_str.c_str(), 0);

		return (int)m_attributes.size();
	}

	RoxShaderCodeParser::Variable RoxShaderCodeParser::getAttribute(int idx) const
	{
		if (idx < 0 || idx >= (int)m_attributes.size())
			return RoxShaderCodeParser::Variable();

		return m_attributes[idx];
	}

	int RoxShaderCodeParser::getOutCount()
	{
		if (m_out.empty())
			parseOut(false);

		return (int)m_out.size();
	}

	RoxShaderCodeParser::Variable RoxShaderCodeParser::getOut(int idx) const
	{
		if (idx < 0 || idx >= (int)m_out.size())
			return RoxShaderCodeParser::Variable();

		return m_out[idx];
	}

	bool RoxShaderCodeParser::fixPerComponentFunctions()
	{
		const char* functions[] = {"pow", "sqrt"}; //ToDo
		int functions_args[] = {2, 1};

		std::string prefix;
		char buf[255];
		for (size_t i = 0; i < sizeof(functions) / sizeof(functions[0]); ++i)
		{
			const char* f = functions[i];
			if (!replaceVariable(f, (m_replace_str + f).c_str()))
				continue;

			const char* types[] = {"float", "vec2", "vec3", "vec4"};
			for (int j = 0; j < 4; ++j)
			{
				const char* t = types[j];
				prefix.append(t + (" " + m_replace_str) + f + "(");
				for (int k = 0; k < functions_args[i]; ++k)
				{
					printf(buf, "%s%s a%d", k == 0 ? "" : ",", t, k);
					prefix.append(buf);
				}

				prefix.append(std::string("){return ") + (j == 0 ? "" : t) + "(");
				const char* components[] = {".x", ".y", ".z", ".w"};
				for (int k = 0; k < j + 1; ++k)
				{
					prefix.append(std::string(k == 0 ? "" : ",") + f + "(");
					for (int l = 0; l < functions_args[i]; ++l)
					{
						printf(buf, "%sa%d%s", l == 0 ? "" : ",", l, j == 0 ? "" : components[k]);
						prefix.append(buf);
					}
					prefix.append(")");
				}
				prefix.append(");}\n");
			}
		}

		m_code.insert(0, prefix);
		return true;
	}

	void RoxShaderCodeParser::removeComments()
	{
		while (m_code.find("//") != std::string::npos)
		{
			const size_t from = m_code.find("//");
			m_code.erase(from, m_code.find_first_of("\n\r", from + 1) - from);
		}
		while (m_code.find("/*") != std::string::npos)
		{
			const size_t from = m_code.find("/*");
			m_code.erase(from, (m_code.find("*/", from + 2) - from) + 2);
		}
	}

	template <typename t>
	static bool parseVars(std::string& code, std::string& error, t& vars, const char* str, bool remove)
	{
		const size_t str_len = strlen(str);
		for (size_t i = code.find(str); i != std::string::npos; i = code.find(str, i))
		{
			size_t type_from = i + str_len + 1;
			while (code[type_from] <= ' ') if (++type_from >= code.length()) return false;
			size_t type_to = type_from;
			while (code[type_to] > ' ') if (++type_to >= code.length()) return false;

			size_t name_from = type_to + 1;
			while (code[name_from] <= ' ') if (++name_from >= code.length()) return false;
			size_t name_to = name_from;
			while (code[name_to] > ' ' && code[name_to] != ';' && code[name_to] != '[') if (++name_to >= code.length())
				return false;

			const std::string name = code.substr(name_from, name_to - name_from);
			const std::string type_name = code.substr(type_from, type_to - type_from);

			size_t last = name_to;
			while (code[last] != ';')
			{
				if (++last >= code.length())
				{
					error.append("unclosed ; on variable declaration\n");
					return false;
				}
			}

			int count = 1;
			size_t array_from = code.find('[', name_to);
			if (array_from < last)
				count = atoi(&code[array_from + 1]);

			if (count <= 0)
				return false;

			if (type_name.compare(0, 3, "vec") == 0)
			{
				char dim = (type_name.length() == 4) ? type_name[3] : '\0';
				switch (dim)
				{
				case '2': vars.push_back(
						RoxShaderCodeParser::Variable(RoxShaderCodeParser::TYPE_VECTOR2, name.c_str(), count));
					break;
				case '3': vars.push_back(
						RoxShaderCodeParser::Variable(RoxShaderCodeParser::TYPE_VECTOR3, name.c_str(), count));
					break;
				case '4': vars.push_back(
						RoxShaderCodeParser::Variable(RoxShaderCodeParser::TYPE_VECTOR4, name.c_str(), count));
					break;
				default: return false;
				};
			}
			else if (type_name.compare(0, 3, "mat") == 0)
			{
				char dim = (type_name.length() == 4) ? type_name[3] : '\0';
				switch (dim)
				{
				case '2': vars.push_back(
						RoxShaderCodeParser::Variable(RoxShaderCodeParser::TYPE_MATRIX2, name.c_str(), count));
					break;
				case '3': vars.push_back(
						RoxShaderCodeParser::Variable(RoxShaderCodeParser::TYPE_MATRIX3, name.c_str(), count));
					break;
				case '4': vars.push_back(
						RoxShaderCodeParser::Variable(RoxShaderCodeParser::TYPE_MATRIX4, name.c_str(), count));
					break;
				default: return false;
				};
			}
			else if (type_name == "float")
				vars.push_back(RoxShaderCodeParser::Variable(RoxShaderCodeParser::TYPE_FLOAT, name.c_str(), count));
			else if (type_name == "sampler2D")
				vars.push_back(RoxShaderCodeParser::Variable(RoxShaderCodeParser::TYPE_SAMPLER2D, name.c_str(), count));
			else if (type_name == "samplerCube")
				vars.push_back(
					RoxShaderCodeParser::Variable(RoxShaderCodeParser::TYPE_SAMPLER_CUBE, name.c_str(), count));
			else
				return false;

			if (remove)
				code.erase(i, last - i + 1);
			else
				i = last + 1;
		}

		return true;
	}

	bool RoxShaderCodeParser::parseUniforms(bool remove)
	{
		return parseVars(m_code, m_error, m_uniforms, "uniform", remove);
	}

	bool RoxShaderCodeParser::parseVarying(bool remove)
	{
		return parseVars(m_code, m_error, m_varying, "varying", remove);
	}

	bool RoxShaderCodeParser::parseOut(bool remove) { return parseVars(m_code, m_error, m_out, "out", remove); }

	bool RoxShaderCodeParser::parsePredefinedUniforms(const char* replace_prefix_str, bool replace)
	{
		if (!replace_prefix_str)
			return false;

		const char* gl_matrix_names[] = {"gl_ModelViewMatrix", "gl_ModelViewProjectionMatrix", "gl_ProjectionMatrix"};

		for (size_t i = 0; i < sizeof(gl_matrix_names) / sizeof(gl_matrix_names[0]); ++i)
		{
			std::string to = std::string(replace_prefix_str) + std::string(gl_matrix_names[i] + 3); //strlen("gl_")
			if (replace)
			{
				if (replaceVariable(gl_matrix_names[i], to.c_str()))
					pushUniqueToVector(m_uniforms, Variable(TYPE_MATRIX4, to.c_str(), 1));
			}
			else if (findVariable(gl_matrix_names[i]))
				pushUniqueToVector(m_uniforms, Variable(TYPE_MATRIX4, to.c_str(), 1));
		}

		if (!m_flip_y_uniform.empty())
			pushUniqueToVector(m_uniforms, Variable(TYPE_FLOAT, m_flip_y_uniform.c_str(), 1));

		return true;
	}

	bool RoxShaderCodeParser::parseAttributes(const char* info_replace_str, const char* code_replace_str)
	{
		if (!info_replace_str)
			return false;

		const char* gl_attr_names[] = {"gl_Vertex", "gl_Normal", "gl_Color"};
		VARIABLE_TYPE gl_attr_types[] = {TYPE_VECTOR4, TYPE_VECTOR3, TYPE_VECTOR4};

		for (size_t i = 0; i < sizeof(gl_attr_names) / sizeof(gl_attr_names[0]); ++i)
		{
			const std::string info = std::string(info_replace_str) + std::string(gl_attr_names[i] + 3); //strlen("gl_")
			if (code_replace_str)
			{
				const std::string replace = std::string(code_replace_str) + std::string(gl_attr_names[i] + 3);
				//strlen("gl_")
				if (replaceVariable(gl_attr_names[i], replace.c_str()))
					pushUniqueToVector(m_attributes, Variable(gl_attr_types[i], info.c_str(), 0));
			}
			else
			{
				if (m_code.find(gl_attr_names[i], 0) != std::string::npos)
					pushUniqueToVector(m_attributes, Variable(gl_attr_types[i], info.c_str(), 0));
			}
		}

		const char* tc_atr_name = "gl_MultiTexCoord";

		size_t start_pos = 0;
		while ((start_pos = m_code.find(tc_atr_name, start_pos)) != std::string::npos)
		{
			const size_t replace_start = start_pos;
			start_pos += strlen(tc_atr_name);
			const int idx = atoi(&m_code[start_pos]);

			if (code_replace_str)
				m_code.replace(replace_start, 3, code_replace_str); //strlen("gl_")

			char buf[255];
			printf(buf, "%s%s%d", info_replace_str, tc_atr_name + 3, idx);
			pushUniqueToVector(m_attributes, Variable(TYPE_VECTOR4, buf, idx));
		}

		return true;
	}

	//TODO: UPDATE LATER --- Verry Important
	bool RoxShaderCodeParser::replaceHlslTypes()
	{
		replaceVariable("vec2", "float2");
		replaceVariable("vec3", "float3");
		replaceVariable("vec4", "float4");
		replaceVariable("mat2", "float2x2");
		replaceVariable("mat3", "float3x3");
		replaceVariable("mat4", "float4x4");
		return true;
	}

	static size_t getVarPos(const std::string& code, size_t pos, int add)
	{
		int brace_count = 0;
		char lbrace = add > 0 ? '(' : ')';
		char rbrace = add > 0 ? ')' : '(';
		bool first_spaces = true;

		for (size_t i = pos + add; i < code.size() && i > 0; i += add)
		{
			const char c = code[i];
			if (first_spaces && c <= ' ')
				continue;

			first_spaces = false;

			if (c == lbrace)
			{
				++brace_count;
				continue;
			}

			if (c == rbrace)
			{
				--brace_count;
				if (brace_count < 0)
					return add > 0 ? i : i - add;

				continue;
			}

			if (brace_count)
				continue;

			if (strchr(";+-=*/,<>%?&|:{} \t\n\r", c))
				return add > 0 ? i : i - add;
		}

		return pos;
	}

	static bool isNumericOnlyVar(const std::string& s)
	{
		for (size_t i = 0; i < s.size(); ++i) if (isalpha(s[i]) || s[i] == '_') return false;
		return true;
	}

	static void removeVarSpaces(std::string& s)
	{
		if (s.empty())
			return;

		size_t f, t = 0;
		for (f = 0; f < s.size(); ++f) if (s[f] > ' ') break;
		for (t = s.size(); t > 0; --t) if (s[t - 1] > ' ') break;

		s = s.substr(f, t);
	}

	bool RoxShaderCodeParser::replaceHlslMul(const char* func_name)
	{
		if (!func_name)
			return false;

		typedef std::pair<size_t, size_t> scope;
		scope global_scope = scope(0, std::string::npos);
		typedef std::map<std::string, scope> mat_map;
		mat_map matrices;
		for (int i = 0; i < (int)m_varying.size(); ++i) if (m_varying[i].type == TYPE_MATRIX4) matrices[m_varying[i].
			name] = global_scope;
		for (int i = 0; i < (int)m_uniforms.size(); ++i) if (m_uniforms[i].type == TYPE_MATRIX4) matrices[m_uniforms[i].
			name] = global_scope;

		size_t start_pos = 0;
		while ((start_pos = m_code.find("mat", start_pos)) != std::string::npos)
		{
			if (start_pos != 0 && isNameChar(m_code[start_pos - 1]))
			{
				start_pos += 3; //strlen("mat")
				continue;
			}

			if (start_pos + 5 >= m_code.size()) //strlen("mat4 ")
				break;

			start_pos += 3; //strlen("mat")
			if (!strchr("234", m_code[start_pos]))
				continue;

			if (isNameChar(m_code[++start_pos]))
				continue;

			const size_t end_pos = m_code.find(';', start_pos);
			if (end_pos == std::string::npos)
				break;

			std::string var = m_code.substr(start_pos, end_pos - start_pos);
			const size_t end_pos2 = var.find('=');
			if (end_pos2 < end_pos)
				var.resize(end_pos2);
			removeVarSpaces(var);
			matrices[var] = std::make_pair(end_pos, std::string::npos); //ToDo: find right scope border
		}

		bool result = false;
		start_pos = 0;
		while ((start_pos = m_code.find('*', start_pos)) != std::string::npos)
		{
			const size_t left = getVarPos(m_code, start_pos, -1);
			const size_t right = getVarPos(m_code, start_pos, 1);
			if (left == start_pos || right == start_pos)
			{
				m_error.append("unable to parse variables in '*' to 'mul' replacement\n");
				return false;
			}

			std::string left_var = m_code.substr(left, start_pos - left);
			std::string right_var = m_code.substr(start_pos + 1, right - start_pos - 1);

			removeVarSpaces(right_var);
			if (right_var.empty()) // *=
			{
				++start_pos;
				continue;
			}

			removeVarSpaces(left_var);

			//ToDo: not sure if matrix*scalar don't need mul, in case of fail replace || with &&
			if (isNumericOnlyVar(left_var) || isNumericOnlyVar(right_var))
			{
				++start_pos;
				continue;
			}

			mat_map::const_iterator left_scope = matrices.find(left_var);
			mat_map::const_iterator right_scope = matrices.find(right_var);
			if ((left_scope == matrices.end() || left_scope->second.first > start_pos || left_scope->second.second <
					start_pos) &&
				(right_scope == matrices.end() || right_scope->second.first > start_pos || right_scope->second.second <
					start_pos))
			{
				//ToDo: detect mul(mat,mat) as mat
				//ToDo: detect mat() constructors

				++start_pos;
				continue;
			}

			const std::string replace = std::string(func_name) + "(" + left_var + "," + right_var + ")";
			m_code.replace(left, right - left, replace);
			result = true;
		}

		return result;
	}

	bool RoxShaderCodeParser::replaceVecFromFloat(const char* func_name)
	{
		if (!func_name)
			return false;

		bool result = false;
		size_t start_pos = 0;
		while ((start_pos = m_code.find("vec", start_pos)) != std::string::npos)
		{
			if (start_pos > 0 && isNameChar(m_code[start_pos - 1]))
			{
				start_pos += 3;
				continue;
			}

			if (start_pos + 4 > m_code.size()) //strlen("vec")+1
			{
				m_error.append("incomplete vec declaration: code end spotted\n");
				return false;
			}

			const char dim = m_code[start_pos + 3];
			if (!strchr("234", dim))
			{
				start_pos += 3;
				continue;
			}

			size_t brace_start = start_pos + 4;

			while (m_code[brace_start] <= ' ') if (++brace_start >= m_code.length()) return false;
			if (m_code[brace_start] != '(')
			{
				start_pos += 3;
				continue;
			}

			bool has_comma = false;
			int brace_count = 0;
			size_t brace_end = brace_start;
			while (++brace_end < m_code.length())
			{
				char c = m_code[brace_end];
				if (c == '(')
				{
					++brace_count;
					continue;
				}

				if (c == ')')
				{
					if (--brace_count < 0) break;
					continue;
				}

				if (!brace_count && c == ',')
				{
					has_comma = true;
					break;
				}
			}

			if (!has_comma)
			{
				std::string replace = func_name + std::string(1, dim) + "(" + m_code.substr(
					brace_start + 1, brace_end - brace_start - 1) + ")";
				m_code.replace(start_pos, brace_end + 1 - start_pos, replace);
				result = true;
			}

			start_pos += 3;
		}

		return result;
	}

	bool RoxShaderCodeParser::replaceMainFunctionHeader(const char* replace_str)
	{
		if (!replace_str)
			return false;

		size_t start_pos = 0;
		while ((start_pos = m_code.find("void", start_pos)) != std::string::npos)
		{
			const size_t lbrace = m_code.find('(', start_pos + 9); //strlen("void main")
			if (lbrace == std::string::npos)
			{
				m_error.append("can't find '(' in void function declaration\n");
				return false;
			}

			std::string main;
			for (size_t i = start_pos + 5; i < lbrace; ++i) //strlen("void ")
			{
				if (m_code[i] > ' ')
					main += m_code[i];
			}

			if (main != "main")
			{
				start_pos += 4; //strlen("void")
				continue;
			}

			const size_t rbrace = m_code.find(')', lbrace);
			if (rbrace == std::string::npos)
			{
				m_error.append("can't find ')' in void function declaration\n");
				return false;
			}

			m_code.replace(start_pos, rbrace + 1 - start_pos, replace_str);
			return true;
		}

		return false;
	}

	bool RoxShaderCodeParser::replaceVariable(const char* from, const char* to, size_t start_pos)
	{
		if (!from || !from[0] || !to)
			return false;

		bool result = false;
		const size_t from_len = strlen(from);
		while ((start_pos = m_code.find(from, start_pos)) != std::string::npos)
		{
			if ((start_pos != 0 && isNameChar(m_code[start_pos - 1])) ||
				(start_pos + from_len < m_code.size() && isNameChar(m_code[start_pos + from_len])))
			{
				start_pos += from_len;
				continue;
			}

			m_code.replace(start_pos, from_len, to);
			start_pos += strlen(to);
			result = true;
		}

		return result;
	}

	bool RoxShaderCodeParser::findVariable(const char* str, size_t start_pos)
	{
		if (!str)
			return false;

		const size_t str_len = strlen(str);
		while ((start_pos = m_code.find(str, start_pos)) != std::string::npos)
		{
			if ((start_pos != 0 && isNameChar(m_code[start_pos - 1])) ||
				(start_pos + str_len < m_code.size() && isNameChar(m_code[start_pos + str_len])))
			{
				start_pos += str_len;
				continue;
			}

			return true;
		}

		return false;
	}
}
