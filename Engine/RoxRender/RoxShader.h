//nya-engine (C) nyan.developer@gmail.com released under the MIT license (see LICENSE)

#pragma once

#include "RoxShaderCodeParser.h"
#include <string>
#include <vector>

namespace RoxRender
{

class RoxShader
{
public:
	enum PROGRAM_TYPE
	{
		VERTEX,
		PIXEL,
		GEOMETRY,
		PROGRAM_TYPES_COUNT
	};

    bool addProgram(PROGRAM_TYPE type,const char*code);

public:
    void bind() const;
    static void unbind();

public:
    int getSamplerLayer(const char *name) const;

public:
	enum UNIFORM_TYPE
	{
		UNIFORM_NOT_FOUND = RoxShaderCodeParser::TYPE_INVALID,
		UNIFORM_FLOAT = RoxShaderCodeParser::TYPE_FLOAT,
		UNIFORM_VEC2 = RoxShaderCodeParser::TYPE_VECTOR2,
		UNIFORM_VEC3 = RoxShaderCodeParser::TYPE_VECTOR3,
		UNIFORM_VEC4 = RoxShaderCodeParser::TYPE_VECTOR4,
		UNIFORM_MAT4 = RoxShaderCodeParser::TYPE_MATRIX4,
		UNIFORM_SAMPLER2D = RoxShaderCodeParser::TYPE_SAMPLER2D,
		UNIFORM_SAMPLER_CUBE = RoxShaderCodeParser::TYPE_SAMPLER_CUBE
	};

    struct Uniform
    {
        std::string name;
        UNIFORM_TYPE type;
        unsigned int array_size;

        Uniform(): type(UNIFORM_NOT_FOUND),array_size(0) {}
    };

    int get_uniforms_count() const;
    const char *get_uniform_name(int idx) const;
    int find_uniform(const char *name) const;
    UNIFORM_TYPE get_uniform_type(int idx) const;
    unsigned int get_uniform_array_size(int idx) const;

public:
    void set_uniform(int idx,float f0,float f1=0.0f,float f2=0.0f,float f3=0.0f) const;
    void set_uniform3_array(int idx,const float *f,unsigned int count) const;
    void set_uniform4_array(int idx,const float *f,unsigned int count) const;
    void set_uniform16_array(int idx,const float *f,unsigned int count) const;

public:
    void release();

public:
    RoxShader(): m_shdr(-1),m_buf(-1) {}

private:
    int m_shdr;
    int m_buf;
    std::vector<Uniform> m_uniforms;
    std::string m_code[PROGRAM_TYPES_COUNT];
};

class compiled_shader
{
public:
    void *get_data() { if(m_data.empty()) return 0; return &m_data[0]; }
    const void *get_data() const { if(m_data.empty()) return 0; return &m_data[0]; }
    size_t get_size() const { return m_data.size(); }

public:
    compiled_shader() {}
    compiled_shader(size_t size) { m_data.resize(size); }

private:
    std::vector<char> m_data;
};

class RoxCompiledShadersProvider
{
public:
    virtual bool get(const char *text,compiled_shader &shader) { return 0; }
    virtual bool set(const char *text,const compiled_shader &shader) { return false; }
};

void set_compiled_shaders_provider(RoxCompiledShadersProvider *provider);

}