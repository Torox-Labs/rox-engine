// Updated By the ROX_ENGINE
// Copyright (C) 2024 Torox Project
// Portions Copyright (C) 2013 nyan.developer@gmail.com (nya-engine)
//
// This file was modified by the Torox Project.
// Update the render api intefrace to check Metal 1th.
//
// This file incorporates code from the nya-engine project, which is licensed under the MIT License.
// See the LICENSE-MIT file in the root directory for more information.
//
// This file is also part of the Rox-engine, which is licensed under a dual-license system:
// 1. Free Use License (for non-commercial and commercial use under specific conditions)
// 2. Commercial License (for use on proprietary platforms)
// See the LICENSE file in the root directory for the full Rox-engine license terms.

#pragma once

#include <string>
#include <vector>
#include <map>

namespace RoxRender
{

class RoxShaderCodeParser
{
public:
    const char *getCode() const { return m_code.c_str(); }
    const char *getError() const { return m_error.c_str(); }

public:
    bool convertToHlsl();
    bool convertToGlsl();
    bool convertToGlsl_es2(const char *precision="mediump");
    bool convertToGlsl3();
    bool convertToMetal();

public:
	enum VARIABLE_TYPE
	{
		TYPE_INVALID = -1,
		TYPE_FLOAT,
		TYPE_VECTOR2,
        TYPE_VECTOR3,
        TYPE_VECTOR4,
		TYPE_MATRIX2,
        TYPE_MATRIX3,
        TYPE_MATRIX4,
		TYPE_SAMPLER2D,
		TYPE_SAMPLER_CUBE
	};

    struct variable
    {
        VARIABLE_TYPE type;
        std::string name;
        union { unsigned int array_size,idx; };

        variable():type(TYPE_INVALID),array_size(0){}
        variable(VARIABLE_TYPE type,const char *name,unsigned int array_size):
                 type(type),name(name?name:""),array_size(array_size) {}
        bool operator < (const variable &v) const { return name<v.name; }
    };

    int getUniformsCount();
    variable getUniform(int idx) const;

    int getAttributesCount();
    variable getAttribute(int idx) const;

    int getOutCount();
    variable getOut(int idx) const;

public:
    bool fixPerComponentFunctions();

public:
    RoxShaderCodeParser(const char *text,const char *replace_prefix_str="_nya_",const char *flip_y_uniform=0):
                       m_code(text?text:""),m_replace_str(replace_prefix_str?replace_prefix_str:""),
                       m_flip_y_uniform(flip_y_uniform?flip_y_uniform:"") { removeComments(); }
private:
    void removeComments();

    bool parseUniforms(bool remove);
    bool parsePredefined_uniforms(const char *replace_prefix_str,bool replace);
    bool parseAttributes(const char *info_replace_str,const char *code_replace_str);
    bool parseVarying(bool remove);
    bool parseOut(bool remove);

    bool replaceMainFunctionHeader(const char *replace_str);
    bool replaceVecFromFloat(const char *func_name);
    bool replaceHlslTypes();
    bool replaceHlslMul(const char *func_name);
    bool replaceVariable(const char *from,const char *to,size_t start_pos=0);
    bool findVariable(const char *str,size_t start_pos=0);

private:
    std::string m_code,m_replace_str,m_flip_y_uniform,m_error;
    std::vector<variable> m_uniforms,m_attributes,m_varying,m_out;
};

}
