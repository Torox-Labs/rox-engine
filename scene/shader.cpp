//nya-engine (C) nyan.developer@gmail.com released under the MIT license (see LICENSE)

#include "shader.h"
#include "scene.h"
#include "camera.h"
#include "memory/invalid_object.h"
#include "transform.h"
#include "render/render.h"
#include "render/screen_quad.h"
#include "render/fbo.h"
#include "formats/text_parser.h"
#include "formats/math_expr_parser.h"
#include <string.h>
#include <stdio.h>
#include <cstdlib>

namespace nya_scene
{

namespace
{

class skeleton_blit_class
{
public:
    void init()
    {
        if(is_valid())
            return;

        char text[1024];

        sprintf(text,"varying float idx;"
                     "void main(){"
                     "idx=gl_MultiTexCoord0.x*%d.0;"
                     "gl_Position=gl_Vertex;}",array_size);

        m_sh3.add_program(nya_render::shader::vertex,text);
        m_sh4.add_program(nya_render::shader::vertex,text);

        const char ps[]="uniform vec%d data[%d];"
                        "varying float idx;"
                        "void main(){"
                        "gl_FragColor=vec4(data[int(idx)]%s);}";

        sprintf(text,ps,3,array_size,",0.0");
        m_sh3.add_program(nya_render::shader::pixel,text);

        sprintf(text,ps,4,array_size,"");
        m_sh4.add_program(nya_render::shader::pixel,text);

        m_data_uniform=m_sh3.find_uniform("data");
        m_quad.init();
    }

    bool is_valid() const { return m_quad.is_valid(); }

    void blit(nya_render::texture &dst,const float *data,int count,int dimention)
    {
        const nya_render::fbo prev_fbo=nya_render::fbo::get_current();
        m_fbo.set_color_target(dst);
        m_fbo.bind();
        const nya_render::rect oldvp=nya_render::get_viewport();
        const nya_render::rect oldsc=nya_render::scissor::get();
        const bool issc=nya_render::scissor::is_enabled();
        if(issc)
            nya_render::scissor::disable();

        static nya_render::state s,oldstate;
        oldstate=nya_render::get_state();
        set_state(s);

        const nya_render::shader &sh=dimention==3?m_sh3:m_sh4;

        for(int offset=0;offset<count;offset+=array_size)
        {
            int size=count-offset;
            if(size>array_size)
                size=array_size;

            if(dimention==3)
                sh.set_uniform3_array(m_data_uniform,data+offset*3,size);
            else
                sh.set_uniform4_array(m_data_uniform,data+offset*4,size);

            sh.bind();
            nya_render::set_viewport(offset,0,array_size,1);
            m_quad.draw();
            sh.unbind();
        }

        set_state(oldstate);
        m_fbo.unbind();
        prev_fbo.bind();
        static nya_render::texture notex;
        m_fbo.set_color_target(notex);
        set_viewport(oldvp);

        if(issc)
            nya_render::scissor::enable(oldsc);
    }
    
    void release()
    {
        m_sh3.release();
        m_sh4.release();
        m_quad.release();
        m_fbo.release();
    }
    
private:
    nya_render::shader m_sh3,m_sh4;
    nya_render::fbo m_fbo;
    nya_render::screen_quad m_quad;
    int m_data_uniform;
    static const int array_size=17;

} skeleton_blit;

struct shader_description
{
    struct predefined
    {
        std::string name;
        shared_shader::transform_type transform;
    };

    predefined predefines[shared_shader::predefines_count];

    typedef std::map<std::string,std::string> strings_map;
    strings_map samplers;
    strings_map uniforms;

    std::vector<std::pair<std::string,nya_formats::math_expr_parser> > procedural;

    std::string vertex;
    std::string pixel;
};

shared_shader::transform_type transform_from_string(const char *str)
{
    if(!str)
        return shared_shader::none;

    if(strcmp(str,"local")==0)
        return shared_shader::local;

    if(strcmp(str,"local_rot")==0)
        return shared_shader::local_rot;

    if(strcmp(str,"local_rot_scale")==0)
        return shared_shader::local_rot_scale;

    return shared_shader::none;
}

}

bool load_nya_shader_internal(shared_shader &res,shader_description &desc,resource_data &data,const char* name,bool include)
{
    nya_formats::text_parser parser;
    parser.load_from_data((const char *)data.get_data(),data.get_size());
    for(int section_idx=0;section_idx<parser.get_sections_count();++section_idx)
    {
        if(parser.is_section_type(section_idx,"include"))
        {
            const char *file=parser.get_section_name(section_idx);
            if(!file || !file[0])
            {
                log()<<"unable to load include in shader "<<name<<": invalid filename\n";
                return false;
            }

            std::string path(name?name:"");
            size_t p=path.rfind("/");
            if(p==std::string::npos)
                p=path.rfind("\\");

            if(p==std::string::npos)
                path.clear();
            else
                path.resize(p+1);

            path.append(file);

            nya_resources::resource_data *file_data=nya_resources::get_resources_provider().access(path.c_str());
            if(!file_data)
            {
                log()<<"unable to load include resource in shader "<<name<<": unable to access resource "<<path.c_str()<<"\n";
                return false;
            }

            const size_t data_size=file_data->get_size();
            nya_memory::tmp_buffer_ref include_data(data_size);
            file_data->read_all(include_data.get_data());
            file_data->release();

            if(!load_nya_shader_internal(res,desc,include_data,path.c_str(),true))
            {
                log()<<"unable to load include in shader "<<name<<": unknown format or invalid data in "<<path.c_str()<<"\n";
                include_data.free();
                return false;
            }

            include_data.free();
        }
        else if(parser.is_section_type(section_idx,"all"))
        {
            const char *text=parser.get_section_value(section_idx);
            if(text)
            {
                desc.vertex.append(text);
                desc.pixel.append(text);
            }
        }
        else if(parser.is_section_type(section_idx,"sampler"))
        {
            const char *name=parser.get_section_name(section_idx,0);
            const char *semantics=parser.get_section_name(section_idx,1);
            if(!name || !semantics)
            {
                log()<<"unable to load shader "<<name<<": invalid sampler syntax\n";
                return false;
            }

            desc.samplers[semantics]=name;
        }
        else if(parser.is_section_type(section_idx,"vertex"))
        {
            const char *text=parser.get_section_value(section_idx);
            if(text)
                desc.vertex.append(text);
        }
        else if(parser.is_section_type(section_idx,"fragment"))
        {
            const char *text=parser.get_section_value(section_idx);
            if(text)
                desc.pixel.append(text);
        }
        else if(parser.is_section_type(section_idx,"predefined"))
        {
            const char *name=parser.get_section_name(section_idx,0);
            const char *semantics=parser.get_section_name(section_idx,1);
            if(!name || !semantics)
            {
                log()<<"unable to load shader "<<name<<": invalid predefined syntax\n";
                return false;
            }

            //compatibility crutch, will be removed
            if(strcmp(semantics,"nya camera position")==0)
                semantics = "nya camera pos";
            else if(strcmp(semantics,"nya camera rotation")==0)
                semantics = "nya camera rot";

            const char *predefined_semantics[]={"nya camera pos","nya camera rot","nya camera dir",
                                                "nya bones pos","nya bones pos transform","nya bones rot","nya bones rot transform",
                                                "nya bones pos texture","nya bones pos transform texture","nya bones rot texture",
                                                "nya viewport","nya model pos","nya model rot","nya model scale"};

            char predefined_count_static_assert[sizeof(predefined_semantics)/sizeof(predefined_semantics[0])
                                                ==shared_shader::predefines_count?1:-1];
            predefined_count_static_assert[0]=0;
            for(int i=0;i<shared_shader::predefines_count;++i)
            {
                if(strcmp(semantics,predefined_semantics[i])==0)
                {
                    desc.predefines[i].name=name;
                    desc.predefines[i].transform=transform_from_string(parser.get_section_option(section_idx));

                    if(i==shared_shader::bones_pos_tex || i==shared_shader::bones_pos_tr_tex)
                    {
                        skeleton_blit.init();
                        res.texture_buffers.allocate();
                        res.texture_buffers->skeleton_pos_max_count=int(parser.get_section_value_vec4(section_idx).x);
                        res.texture_buffers->skeleton_pos_texture.build_texture(0,res.texture_buffers->skeleton_pos_max_count,1,nya_render::texture::color_rgb32f,1);
                        res.texture_buffers->skeleton_pos_texture.set_filter(nya_render::texture::filter_nearest,
                                                                             nya_render::texture::filter_nearest,nya_render::texture::filter_nearest);
                    }
                    else if(i==shared_shader::bones_rot_tex)
                    {
                        skeleton_blit.init();
                        res.texture_buffers.allocate();
                        res.texture_buffers->skeleton_rot_max_count=int(parser.get_section_value_vec4(section_idx).x);
                        res.texture_buffers->skeleton_rot_texture.build_texture(0,res.texture_buffers->skeleton_rot_max_count,1,nya_render::texture::color_rgba32f,1);
                        res.texture_buffers->skeleton_rot_texture.set_filter(nya_render::texture::filter_nearest,
                                                                             nya_render::texture::filter_nearest,nya_render::texture::filter_nearest);
                    }
                    break;
                }
            }
        }
        else if(parser.is_section_type(section_idx,"uniform"))
        {
            const char *name=parser.get_section_name(section_idx,0);
            const char *semantics=parser.get_section_name(section_idx,1);
            if(!name || !semantics)
            {
                log()<<"unable to load shader "<<name<<": invalid uniform syntax\n";
                return false;
            }

            desc.uniforms[semantics]=name;
            res.uniforms.resize(res.uniforms.size()+1);
            res.uniforms.back().name=semantics;
            res.uniforms.back().transform=transform_from_string(parser.get_section_option(section_idx));
            res.uniforms.back().default_value=parser.get_section_value_vec4(section_idx);
        }
        else if(parser.is_section_type(section_idx,"procedural"))
        {
            const char *name=parser.get_section_name(section_idx,0);
            nya_formats::math_expr_parser p;
            if(!name || !name[0] || !p.parse(parser.get_section_value(section_idx)))
            {
                log()<<"unable to load shader "<<name<<": invalid procedural\n";
                return false;
            }

            desc.procedural.push_back(std::make_pair(name,p));
        }
        else
            log()<<"scene shader load warning: unsupported shader tag "<<parser.get_section_type(section_idx)<<" in "<<name<<"\n";
    }

    if(include)
        return true;

    if(desc.vertex.empty())
    {
        log()<<"scene shader load error: empty vertex shader in "<<name<<"\n";
        return false;
    }

    if(desc.pixel.empty())
    {
        log()<<"scene shader load error: empty pixel shader in "<<name<<"\n";
        return false;
    }

    //log()<<"vertex <"<<res.vertex.c_str()<<">\n";
    //log()<<"pixel <"<<res.pixel.c_str()<<">\n";

    if(!res.shdr.add_program(nya_render::shader::vertex,desc.vertex.c_str()))
        return false;

    if(!res.shdr.add_program(nya_render::shader::pixel,desc.pixel.c_str()))
        return false;

    for(int i=0;i<shared_shader::predefines_count;++i)
    {
        const shader_description::predefined &p=desc.predefines[i];
        if(p.name.empty())
            continue;

        res.predefines.resize(res.predefines.size()+1);
        res.predefines.back().type=(shared_shader::predefined_values)i;
        if(i==shared_shader::bones_pos_tex || i==shared_shader::bones_pos_tr_tex || i==shared_shader::bones_rot_tex)
        {
            res.predefines.back().location=res.shdr.get_sampler_layer(p.name.c_str());
            continue;
        }

        res.predefines.back().transform=p.transform;
        res.predefines.back().location=res.shdr.find_uniform(p.name.c_str());
    }

    for(int i=0;i<(int)res.uniforms.size();++i)
    {
        const int l=res.uniforms[i].location=res.shdr.find_uniform(desc.uniforms[res.uniforms[i].name].c_str());
        const nya_math::vec4 &v=res.uniforms[i].default_value;
        res.shdr.set_uniform(l,v.x,v.y,v.z,v.w);
    }

    for(shader_description::strings_map::const_iterator it=desc.samplers.begin();
        it!=desc.samplers.end();++it)
    {
        int layer=res.shdr.get_sampler_layer(it->second.c_str());
        if(layer<0)
            continue;

        if(layer>=(int)res.samplers.size())
            res.samplers.resize(layer+1);

        res.samplers[layer]=it->first;
    }

    for(int i=0;i<(int)desc.procedural.size();++i)
    {
        const int idx=res.shdr.find_uniform(desc.procedural[i].first.c_str());
        if(idx<0)
            continue;

        const int count=(int)res.shdr.get_uniform_array_size(idx);
        nya_formats::math_expr_parser &p=desc.procedural[i].second;
        p.set_var("count",(float)count);

        std::vector<nya_math::vec4> values(count);
        for(int j=0;j<count;++j)
        {
            p.set_var("i",(float)j);
            values[j]=p.calculate_vec4();
        }

        res.shdr.set_uniform4_array(idx,&values[0].x,count);
    }

    return true;
}

bool shader::load_nya_shader(shared_shader &res,resource_data &data,const char* name)
{
    shader_description desc;
    return load_nya_shader_internal(res,desc,data,name,false);
}

bool shader::load(const char *name)
{
    shader_internal::default_load_function(load_nya_shader);
    m_internal.reset_skeleton();
    return m_internal.load(name);
}

void shader::unload()
{
    m_internal.unload();
    if(skeleton_blit.is_valid() && !m_internal.get_shared_resources().get_first_resource().is_valid())
        skeleton_blit.release();
}

void shader::create(const shared_shader &res)
{
    m_internal.reset_skeleton();
    m_internal.create(res);
}

bool shader::build(const char *code_text)
{
    if(!code_text)
        return false;

    shared_shader s;
    nya_memory::tmp_buffer_ref data(strlen(code_text));
    data.copy_from(code_text,data.get_size());
    const bool result=load_nya_shader(s,data,"");
    data.free();
    create(s);
    return result;
}

void shader_internal::set() const
{
    if(!m_shared.is_valid())
        return;

    m_shared->shdr.bind();

    for(size_t i=0;i<m_shared->predefines.size();++i)
    {
        const shared_shader::predefined &p=m_shared->predefines[i];
        switch(p.type)
        {
            case shared_shader::camera_pos:
            {
                if(p.transform==shared_shader::local)
                {
                    const nya_math::vec3 v=transform::get().inverse_transform(get_camera().get_pos());
                    m_shared->shdr.set_uniform(p.location,v.x,v.y,v.z);
                }
                else if(p.transform==shared_shader::local_rot)
                {
                    const nya_math::vec3 v=transform::get().inverse_rot(get_camera().get_pos());
                    m_shared->shdr.set_uniform(p.location,v.x,v.y,v.z);
                }
                else if(p.transform==shared_shader::local_rot_scale)
                {
                    const nya_math::vec3 v=transform::get().inverse_rot_scale(get_camera().get_pos());
                    m_shared->shdr.set_uniform(p.location,v.x,v.y,v.z);
                }
                else
                {
                    const nya_math::vec3 v=get_camera().get_pos();
                    m_shared->shdr.set_uniform(p.location,v.x,v.y,v.z);
                }
            }
            break;

            case shared_shader::camera_dir:
            {
                if(p.transform==shared_shader::local_rot || p.transform==shared_shader::local)
                {
                    const nya_math::vec3 v=transform::get().inverse_rot(get_camera().get_dir());
                    m_shared->shdr.set_uniform(p.location,v.x,v.y,v.z);
                }
                else
                {
                    const nya_math::vec3 v=get_camera().get_dir();
                    m_shared->shdr.set_uniform(p.location,v.x,v.y,v.z);
                }
            }
            break;

            case shared_shader::camera_rot:
            {
                if(p.transform==shared_shader::local_rot || p.transform==shared_shader::local)
                {
                    const nya_math::quat v=transform::get().inverse_transform(get_camera().get_rot());
                    m_shared->shdr.set_uniform(p.location,v.v.x,v.v.y,v.v.z,v.w);
                }
                else
                {
                    const nya_math::quat v=get_camera().get_rot();
                    m_shared->shdr.set_uniform(p.location,v.v.x,v.v.y,v.v.z,v.w);
                }
            }
            break;

            case shared_shader::bones_pos:
            {
                if(m_skeleton && m_shared->last_skeleton_pos!=m_skeleton)
                {
                    m_shared->shdr.set_uniform3_array(p.location,m_skeleton->get_pos_buffer(),m_skeleton->get_bones_count());
                    m_shared->last_skeleton_pos=m_skeleton;
                }
            }
            break;

            case shared_shader::bones_pos_tr:
            {
                if(m_skeleton && m_shared->last_skeleton_pos!=m_skeleton)
                {
                    nya_memory::tmp_buffer_scoped tmp(m_skeleton->get_bones_count()*3*4);
                    nya_math::vec3 *pos=(nya_math::vec3 *)tmp.get_data();

                    if(m_skeleton->has_original_rot())
                    {
                        for(int i=0;i<m_skeleton->get_bones_count();++i)
                        {
                            pos[i]=m_skeleton->get_bone_pos(i)
                                  +(m_skeleton->get_bone_rot(i)*nya_math::quat::invert(m_skeleton->get_bone_original_rot(i)))
                                  .rotate(-m_skeleton->get_bone_original_pos(i));
                        }
                    }
                    else
                    {
                        for(int i=0;i<m_skeleton->get_bones_count();++i)
                        {
                            pos[i]=m_skeleton->get_bone_pos(i)
                                  +m_skeleton->get_bone_rot(i).rotate(-m_skeleton->get_bone_original_pos(i));
                        }
                    }

                    m_shared->shdr.set_uniform3_array(p.location,(float *)tmp.get_data(),m_skeleton->get_bones_count());
                    m_shared->last_skeleton_pos=m_skeleton;
                }
            }
            break;

            case shared_shader::bones_rot:
            {
                if(m_skeleton && m_shared->last_skeleton_rot!=m_skeleton)
                {
                    m_shared->shdr.set_uniform4_array(p.location,m_skeleton->get_rot_buffer(),m_skeleton->get_bones_count());
                    m_shared->last_skeleton_rot=m_skeleton;
                }
            }
            break;

            case shared_shader::bones_rot_tr:
            {
                if(m_skeleton && m_shared->last_skeleton_rot!=m_skeleton)
                {
                    nya_memory::tmp_buffer_scoped tmp(m_skeleton->get_bones_count()*4*4);
                    nya_math::quat *rot=(nya_math::quat *)tmp.get_data();
                    for(int i=0;i<m_skeleton->get_bones_count();++i)
                        rot[i]=m_skeleton->get_bone_rot(i)*nya_math::quat::invert(m_skeleton->get_bone_original_rot(i));

                    m_shared->shdr.set_uniform4_array(p.location,(float *)tmp.get_data(),m_skeleton->get_bones_count());
                    m_shared->last_skeleton_rot=m_skeleton;
                }
            }
            break;

            case shared_shader::bones_pos_tex:
            {
                if(!m_shared->texture_buffers.is_valid())
                    m_shared->texture_buffers.allocate();

                if(m_skeleton && m_shared->texture_buffers->last_skeleton_pos_texture!=m_skeleton && m_skeleton->get_bones_count()>0)
                {
                    skeleton_blit.blit(m_shared->texture_buffers->skeleton_pos_texture,m_skeleton->get_pos_buffer(),m_skeleton->get_bones_count(),3);
                    m_shared->texture_buffers->last_skeleton_pos_texture=m_skeleton;
                }

                m_shared->texture_buffers->skeleton_pos_texture.bind(p.location);
            }
            break;

            case shared_shader::bones_pos_tr_tex:
            {
                if(!m_shared->texture_buffers.is_valid())
                    m_shared->texture_buffers.allocate();

                //ToDo: if(m_skeleton->has_original_rot())

                if(m_skeleton && m_shared->texture_buffers->last_skeleton_pos_texture!=m_skeleton && m_skeleton->get_bones_count()>0)
                {
                    nya_memory::tmp_buffer_scoped tmp(m_skeleton->get_bones_count()*3*4);
                    nya_math::vec3 *pos=(nya_math::vec3 *)tmp.get_data();
                    for(int i=0;i<m_skeleton->get_bones_count();++i)
                        pos[i]=m_skeleton->get_bone_pos(i)+m_skeleton->get_bone_rot(i).rotate(-m_skeleton->get_bone_original_pos(i));

                    skeleton_blit.blit(m_shared->texture_buffers->skeleton_pos_texture,(float *)tmp.get_data(),m_skeleton->get_bones_count(),3);
                    m_shared->texture_buffers->last_skeleton_pos_texture=m_skeleton;
                }

                m_shared->texture_buffers->skeleton_pos_texture.bind(p.location);
            }
            break;

            case shared_shader::bones_rot_tex:
            {
                if(!m_shared->texture_buffers.is_valid())
                    m_shared->texture_buffers.allocate();

                if(m_skeleton && m_shared->texture_buffers->last_skeleton_rot_texture!=m_skeleton && m_skeleton->get_bones_count()>0)
                {
                    skeleton_blit.blit(m_shared->texture_buffers->skeleton_rot_texture,m_skeleton->get_rot_buffer(),m_skeleton->get_bones_count(),4);
                    m_shared->texture_buffers->last_skeleton_rot_texture=m_skeleton;
                }

                m_shared->texture_buffers->skeleton_rot_texture.bind(p.location);
            }
            break;

            case shared_shader::viewport:
            {
                nya_render::rect r=nya_render::get_viewport();
                m_shared->shdr.set_uniform(p.location,float(r.x),float(r.y),float(r.width),float(r.height));
            }
            break;

            case shared_shader::model_pos:
            {
                const nya_math::vec3 v=transform::get().get_pos();
                m_shared->shdr.set_uniform(p.location,v.x,v.y,v.z);
            }
            break;

            case shared_shader::model_rot:
            {
                const nya_math::quat v=transform::get().get_rot();
                m_shared->shdr.set_uniform(p.location,v.v.x,v.v.y,v.v.z,v.w);
            }
            break;

            case shared_shader::model_scale:
            {
                const nya_math::vec3 v=transform::get().get_scale();
                m_shared->shdr.set_uniform(p.location,v.x,v.y,v.z);
            }
            break;

            case shared_shader::predefines_count: break;
        }
    }

    m_shared->shdr.bind();
}

int shader_internal::get_texture_slot(const char *semantics) const
{
    if(!semantics || !m_shared.is_valid())
        return -1;

    for(int i=0;i<(int)m_shared->samplers.size();++i)
    {
        if(m_shared->samplers[i]==semantics)
            return i;
    }

    return -1;
}

const char *shader_internal::get_texture_semantics(int slot) const
{
    if(!m_shared.is_valid() || slot<0 || slot>=(int)m_shared->samplers.size())
        return 0;

    return m_shared->samplers[slot].c_str();
}

int shader_internal::get_texture_slots_count() const
{
    if(!m_shared.is_valid())
        return 0;

    return (int)m_shared->samplers.size();
}

int shader_internal::get_uniform_idx(const char *name) const
{
    if(!m_shared.is_valid() || !name)
        return 0;

    for(int i=0;i<(int)m_shared->uniforms.size();++i)
    {
        if(m_shared->uniforms[i].name==name)
            return i;
    }

    return -1;
}

const shared_shader::uniform &shader_internal::get_uniform(int idx) const
{
    if(!m_shared.is_valid() || idx<0 || idx >=(int)m_shared->uniforms.size())
        return nya_memory::invalid_object<shared_shader::uniform>();

    return m_shared->uniforms[idx];
}

nya_render::shader::uniform_type shader_internal::get_uniform_type(int idx) const
{
    if(!m_shared.is_valid())
        return nya_render::shader::uniform_not_found;

    return m_shared->shdr.get_uniform_type(get_uniform(idx).location);
}

unsigned int shader_internal::get_uniform_array_size(int idx) const
{
    if(!m_shared.is_valid())
        return 0;

    return m_shared->shdr.get_uniform_array_size(get_uniform(idx).location);
}

void shader_internal::set_uniform_value(int idx,float f0,float f1,float f2,float f3) const
{
    if(!m_shared.is_valid() || idx<0 || idx >=(int)m_shared->uniforms.size())
        return;

    if(m_shared->uniforms[idx].location<0)
        return;

    if(m_shared->uniforms[idx].transform==shared_shader::local)
    {
        nya_math::vec3 v=transform::get().inverse_transform(nya_math::vec3(f0,f1,f2));
        m_shared->shdr.set_uniform(m_shared->uniforms[idx].location,v.x,v.y,v.z,f3);
    }
    else if(m_shared->uniforms[idx].transform==shared_shader::local_rot)
    {
        nya_math::vec3 v=transform::get().inverse_rot(nya_math::vec3(f0,f1,f2));
        m_shared->shdr.set_uniform(m_shared->uniforms[idx].location,v.x,v.y,v.z,f3);
    }
    else if(m_shared->uniforms[idx].transform==shared_shader::local_rot_scale)
    {
        nya_math::vec3 v=transform::get().inverse_rot_scale(nya_math::vec3(f0,f1,f2));
        m_shared->shdr.set_uniform(m_shared->uniforms[idx].location,v.x,v.y,v.z,f3);
    }
    else
        m_shared->shdr.set_uniform(m_shared->uniforms[idx].location,f0,f1,f2,f3);
}

void shader_internal::set_uniform4_array(int idx,const float *array,unsigned int count) const
{
    if(!m_shared.is_valid() || idx<0 || idx >=(int)m_shared->uniforms.size())
        return;

    m_shared->shdr.set_uniform4_array(m_shared->uniforms[idx].location,array,count);
}

int shader_internal::get_uniforms_count() const
{
    if(!m_shared.is_valid())
        return 0;

    return (int)m_shared->uniforms.size();
}

const nya_render::skeleton *shader_internal::m_skeleton=0;

void shader_internal::skeleton_changed(const nya_render::skeleton *skeleton) const
{
    if(!m_shared.is_valid())
        return;

    if(skeleton==m_shared->last_skeleton_pos)
        m_shared->last_skeleton_pos=0;

    if(skeleton==m_shared->last_skeleton_rot)
        m_shared->last_skeleton_rot=0;

    if(m_shared->texture_buffers.is_valid())
    {
        if(skeleton==m_shared->texture_buffers->last_skeleton_pos_texture)
            m_shared->texture_buffers->last_skeleton_pos_texture=0;

        if(skeleton==m_shared->texture_buffers->last_skeleton_rot_texture)
            m_shared->texture_buffers->last_skeleton_rot_texture=0;
    }
}

}
