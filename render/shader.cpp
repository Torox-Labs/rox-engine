//nya-engine (C) nyan.developer@gmail.com released under the MIT license (see LICENSE)

#include "shader.h"
#include "shader_code_parser.h"
#include "render.h"
#include "render_api.h"

namespace nya_render
{

bool shader::add_program(program_type type,const char*code)
{
    if(type>=program_types_count)
    {
        log()<<"Unable to add shader program: invalid shader type\n";
        return false;
    }

    if(!code || !code[0])
    {
        log()<<"Unable to add shader program: invalid code\n";
        return false;
    }

    m_code[type]=code;
    if(!m_code[vertex].empty() && !m_code[pixel].empty())
    {
        release();
        m_shdr=get_api_interface().create_shader(m_code[vertex].c_str(), m_code[pixel].c_str());
        if(m_shdr<0)
            return false;

        m_buf=get_api_interface().create_uniform_buffer(m_shdr);

        m_uniforms.resize(get_api_interface().get_uniforms_count(m_shdr));
        for(int i=0;i<(int)m_uniforms.size();++i)
            m_uniforms[i]= get_api_interface().get_uniform(m_shdr,i);
        return true;
    }

    return true;
}

void shader::bind() const { get_api_state().shader=m_shdr; get_api_state().uniform_buffer=m_buf; }
void shader::unbind() { get_api_state().shader= -1; get_api_state().uniform_buffer= -1; }

int shader::get_sampler_layer(const char *name) const
{
    if(!name || !name[0])
        return -1;

    for(int i=0,layer=0;i<(int)m_uniforms.size();++i)
    {
        const uniform &u=m_uniforms[i];
        if(u.type!=uniform_sampler2d && u.type!=uniform_sampler_cube)
            continue;

        if(u.name==name)
            return layer;

        ++layer;
    }

    return -1;
}

int shader::find_uniform(const char *name) const
{
    if(!name || !name[0])
        return -1;

    for(int i=0;i<(int)m_uniforms.size();++i)
    {
        if(m_uniforms[i].name==name)
            return i;
    }

    return -1;
}

void shader::set_uniform(int i,float f0,float f1,float f2,float f3) const
{
    if(i<0 || i>=(int)m_uniforms.size())
        return;

    const float f[]={f0,f1,f2,f3};
    get_api_interface().set_uniform(m_shdr,i,f,4);
}

void shader::set_uniform3_array(int i,const float *f,unsigned int count) const
{
    if(!f || i<0 || i>=(int)m_uniforms.size())
        return;

    const uniform &u=m_uniforms[i];
    if(count>=u.array_size)
        count=u.array_size;

    if(!count)
        return;

    get_api_interface().set_uniform(m_shdr,i,f,count*3);
}

void shader::set_uniform4_array(int i,const float *f,unsigned int count) const
{
    if(!f || i<0 || i>=(int)m_uniforms.size())
        return;

    if(!count)
        return;

    const uniform &u=m_uniforms[i];
    if(u.type<uniform_mat4)
    {
        if(count>=u.array_size)
            count=u.array_size;

        get_api_interface().set_uniform(m_shdr,i,f,count*4);
    }
    else if(u.type==uniform_mat4)
    {
        count/=4;
        if(count>=u.array_size)
            count=u.array_size;

        get_api_interface().set_uniform(m_shdr,i,f,count*16);
    }
}

void shader::set_uniform16_array(int i,const float *f,unsigned int count) const
{
    if(!f || i<0 || i>=(int)m_uniforms.size())
        return;

    const uniform &u=m_uniforms[i];
    if(u.type!=uniform_mat4)
        return;

    if(count>=u.array_size)
        count=u.array_size;

    if(!count)
        return;

    get_api_interface().set_uniform(m_shdr,i,f,count*16);
}

int shader::get_uniforms_count() const { return (int)m_uniforms.size(); }

const char *shader::get_uniform_name(int idx) const
{
    return idx>=0 && idx<(int)m_uniforms.size()?m_uniforms[idx].name.c_str():0;
}

shader::uniform_type shader::get_uniform_type(int idx) const
{
    return idx>=0 && idx<(int)m_uniforms.size()?m_uniforms[idx].type:uniform_not_found;
}

unsigned int shader::get_uniform_array_size(int idx) const
{
    return idx>=0 && idx<(int)m_uniforms.size()?m_uniforms[idx].array_size:0;
}

void shader::release()
{
    if(m_shdr>=0)
    {
        if(get_api_state().shader==m_shdr)
            get_api_state().shader=-1;
        get_api_interface().remove_shader(m_shdr);
    }

    if(m_buf>=0)
    {
        if(get_api_state().uniform_buffer==m_buf)
            get_api_state().uniform_buffer=-1;
        get_api_interface().remove_uniform_buffer(m_buf);
    }

    m_shdr=m_buf=-1;
    m_uniforms.clear();
}

}
