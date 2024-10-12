//nya-engine (C) nyan.developer@gmail.com released under the MIT license (see LICENSE)

#pragma once

#include "render/shader_code_parser.h"
#include <string>
#include <vector>

namespace nya_render
{

class shader
{
public:
    enum program_type
    {
        vertex,
        pixel,
        geometry,
        program_types_count
    };

    bool add_program(program_type type,const char*code);

public:
    void bind() const;
    static void unbind();

public:
    int get_sampler_layer(const char *name) const;

public:
    enum uniform_type
    {
        uniform_not_found = shader_code_parser::type_invalid,
        uniform_float = shader_code_parser::type_float,
        uniform_vec2 = shader_code_parser::type_vec2,
        uniform_vec3 = shader_code_parser::type_vec3,
        uniform_vec4 = shader_code_parser::type_vec4,
        uniform_mat4 = shader_code_parser::type_mat4,
        uniform_sampler2d = shader_code_parser::type_sampler2d,
        uniform_sampler_cube = shader_code_parser::type_sampler_cube
    };

    struct uniform
    {
        std::string name;
        uniform_type type;
        unsigned int array_size;

        uniform(): type(uniform_not_found),array_size(0) {}
    };

    int get_uniforms_count() const;
    const char *get_uniform_name(int idx) const;
    int find_uniform(const char *name) const;
    uniform_type get_uniform_type(int idx) const;
    unsigned int get_uniform_array_size(int idx) const;

public:
    void set_uniform(int idx,float f0,float f1=0.0f,float f2=0.0f,float f3=0.0f) const;
    void set_uniform3_array(int idx,const float *f,unsigned int count) const;
    void set_uniform4_array(int idx,const float *f,unsigned int count) const;
    void set_uniform16_array(int idx,const float *f,unsigned int count) const;

public:
    void release();

public:
    shader(): m_shdr(-1),m_buf(-1) {}

private:
    int m_shdr;
    int m_buf;
    std::vector<uniform> m_uniforms;
    std::string m_code[program_types_count];
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

class compiled_shaders_provider
{
public:
    virtual bool get(const char *text,compiled_shader &shader) { return 0; }
    virtual bool set(const char *text,const compiled_shader &shader) { return false; }
};

void set_compiled_shaders_provider(compiled_shaders_provider *provider);

}
