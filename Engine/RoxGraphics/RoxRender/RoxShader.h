// Updated By the ROX_ENGINE
// Copyright (C) 2024 Torox Project
// Portions Copyright (C) 2013 nyan.developer@gmail.com (nya-engine)
//
// This file was modified by the Torox Project.
// Update the render api interface to check Metal 1th.
//
// This file incorporates code from the nya-engine project, which is licensed under the MIT License.
// See the LICENSE-MIT file in the root directory for more information.
//
// This file is also part of the Rox-engine, which is licensed under a dual-license system:
// 1. Free Use License (for non-commercial and commercial use under specific conditions)
// 2. Commercial License (for use on proprietary platforms)
// See the LICENSE file in the root directory for the full Rox-engine license terms.

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

    int getUniformsCount() const;
    const char *getUniformName(int idx) const;
    int findUniform(const char *name) const;
    UNIFORM_TYPE getUniformType(int idx) const;
    unsigned int getUniformArraySize(int idx) const;

public:
    void setUniform(int idx,float f0,float f1=0.0f,float f2=0.0f,float f3=0.0f) const;
    void setUniform3Array(int idx,const float *f,unsigned int count) const;
    void setUniform4Array(int idx,const float *f,unsigned int count) const;
    void setUniform16Array(int idx,const float *f,unsigned int count) const;

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

class RoxCompiledShader
{
public:
    void *getData() { if(m_data.empty()) return 0; return &m_data[0]; }
    const void *getData() const { if(m_data.empty()) return 0; return &m_data[0]; }
    size_t getSize() const { return m_data.size(); }

public:
    RoxCompiledShader() {}
    RoxCompiledShader(size_t size) { m_data.resize(size); }

private:
    std::vector<char> m_data;
};

class RoxCompiledShadersProvider
{
public:
    virtual bool get(const char *text, RoxCompiledShader &shader) { return 0; }
    virtual bool set(const char *text,const RoxCompiledShader &shader) { return false; }
};

void set_compiled_shaders_provider(RoxCompiledShadersProvider *provider);

}