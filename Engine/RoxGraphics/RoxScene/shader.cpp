//nya-engine (C) nyan.developer@gmail.com released under the MIT license (see LICENSE)

#include "shader.h"
#include "scene.h"
#include "camera.h"
#include "RoxMemory/RoxInvalidObject.h"
#include "transform.h"
#include "RoxRender/RoxRender.h"
#include "RoxRender/RoxScreenQuad.h"
#include "RoxRender/RoxFbo.h"
#include "RoxFormats/RoxTextParser.h"
#include "RoxFormats/RoxMathExprParser.h"

#include <cstring>
#include <cstdio>
#include <cstdlib>

#include "RoxMath/RoxQuaternion.h"

namespace RoxScene
{

namespace
{

class skeleton_blit_class
{
public:
    void init()
    {
        if(isValid())
            return;

        char text[1024];

        sprintf(text,"varying float idx;"
                     "void main(){"
                     "idx=gl_MultiTexCoord0.x*%d.0;"
                     "gl_Position=gl_Vertex;}",array_size);

        m_sh3.addProgram(RoxRender::RoxShader::VERTEX,text);
        m_sh4.addProgram(RoxRender::RoxShader::VERTEX,text);

        const char ps[]="uniform vec%d data[%d];"
                        "varying float idx;"
                        "void main(){"
                        "gl_FragColor=Vector4(data[int(idx)]%s);}";

        sprintf(text,ps,3,array_size,",0.0");
        m_sh3.addProgram(RoxRender::RoxShader::PIXEL,text);

        sprintf(text,ps,4,array_size,"");
        m_sh4.addProgram(RoxRender::RoxShader::PIXEL,text);

        m_data_uniform=m_sh3.findUniform("data");
        m_quad.init();
    }

    bool isValid() const { return m_quad.isValid(); }

    void blit(RoxRender::RoxTexture &dst,const float *data,int count,int dimention)
    {
        const RoxRender::RoxFbo prev_fbo=RoxRender::RoxFbo::getCurrent();
        m_fbo.setColorTarget(dst);
        m_fbo.bind();
        const RoxRender::Rectangle oldvp=RoxRender::getViewport();
        const RoxRender::Rectangle oldsc=RoxRender::Scissor::get();
        const bool issc=RoxRender::Scissor::isEnabled();
        if(issc)
            RoxRender::Scissor::disable();

        static RoxRender::State s,oldstate;
        oldstate=RoxRender::getState();
        setState(s);

        const RoxRender::RoxShader &sh=dimention==3?m_sh3:m_sh4;

        for(int offset=0;offset<count;offset+=array_size)
        {
            int size=count-offset;
            if(size>array_size)
                size=array_size;

            if(dimention==3)
                sh.setUniform3Array(m_data_uniform,data+offset*3,size);
            else
                sh.setUniform4Array(m_data_uniform,data+offset*4,size);

            sh.bind();
            RoxRender::setViewport(offset,0,array_size,1);
            m_quad.draw();
            sh.unbind();
        }

        setState(oldstate);
        m_fbo.unbind();
        prev_fbo.bind();
        static RoxRender::RoxTexture notex;
        m_fbo.setColorTarget(notex);
        setViewport(oldvp);

        if(issc)
            RoxRender::Scissor::enable(oldsc);
    }
    
    void release()
    {
        m_sh3.release();
        m_sh4.release();
        m_quad.release();
        m_fbo.release();
    }
    
private:
    RoxRender::RoxShader m_sh3,m_sh4;
    RoxRender::RoxFbo m_fbo;
    RoxRender::RoxScreenQuad m_quad;
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

    std::vector<std::pair<std::string,RoxFormats::RoxMathExprParser> > procedural;

    std::string VERTEX;
    std::string PIXEL;
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

bool load_nya_shader_internal(shared_shader &res, shader_description &desc, RoxScene::resource_data &data, const char* name, bool include)
{
    RoxFormats::RTextParser parser;
    parser.loadFromData((const char *)data.getData(),data.getSize());
    for(int section_idx=0;section_idx<parser.getSectionsCount();++section_idx)
    {
        if(parser.isSectionType(section_idx,"include"))
        {
            const char *file=parser.getSectionName(section_idx);
            if(!file || !file[0])
            {
                log()<<"unable to load include in RoxShader "<<name<<": invalid filename\n";
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

            RoxResources::RoxResourceData *file_data=RoxResources::getResourcesProvider().access(path.c_str());
            if(!file_data)
            {
                log()<<"unable to load include resource in RoxShader "<<name<<": unable to access resource "<<path.c_str()<<"\n";
                return false;
            }

            const size_t data_size=file_data->getSize();
            RoxMemory::RoxTmpBufferRef include_data(data_size);
            file_data->readAll(include_data.getData());
            file_data->release();

            if(!load_nya_shader_internal(res,desc,include_data,path.c_str(),true))
            {
                log()<<"unable to load include in RoxShader "<<name<<": unknown format or invalid data in "<<path.c_str()<<"\n";
                include_data.free();
                return false;
            }

            include_data.free();
        }
        else if(parser.isSectionType(section_idx,"all"))
        {
            const char *text=parser.getSectionValue(section_idx);
            if(text)
            {
                desc.VERTEX.append(text);
                desc.PIXEL.append(text);
            }
        }
        else if(parser.isSectionType(section_idx,"sampler"))
        {
            const char *name=parser.getSectionName(section_idx,0);
            const char *semantics=parser.getSectionName(section_idx,1);
            if(!name || !semantics)
            {
                log()<<"unable to load RoxShader "<<name<<": invalid sampler syntax\n";
                return false;
            }

            desc.samplers[semantics]=name;
        }
        else if(parser.isSectionType(section_idx,"VERTEX"))
        {
            const char *text=parser.getSectionValue(section_idx);
            if(text)
                desc.VERTEX.append(text);
        }
        else if(parser.isSectionType(section_idx,"fragment"))
        {
            const char *text=parser.getSectionValue(section_idx);
            if(text)
                desc.PIXEL.append(text);
        }
        else if(parser.isSectionType(section_idx,"predefined"))
        {
            const char *name=parser.getSectionName(section_idx,0);
            const char *semantics=parser.getSectionName(section_idx,1);
            if(!name || !semantics)
            {
                log()<<"unable to load RoxShader "<<name<<": invalid predefined syntax\n";
                return false;
            }

            //compatibility crutch, will be removed
            if(strcmp(semantics,"nya camera position")==0)
                semantics = "nya camera pos";
            else if(strcmp(semantics,"nya camera rotation")==0)
                semantics = "nya camera rot";

            const char *predefined_semantics[]={"nya camera pos","nya camera rot","nya camera dir",
                                                "nya bones pos","nya bones pos transform","nya bones rot","nya bones rot transform",
                                                "nya bones pos RoxTexture","nya bones pos transform RoxTexture","nya bones rot RoxTexture",
                                                "nya viewport","nya model pos","nya model rot","nya model scale"};

            char predefined_count_static_assert[sizeof(predefined_semantics)/sizeof(predefined_semantics[0])
                                                ==shared_shader::predefines_count?1:-1];
            predefined_count_static_assert[0]=0;
            for(int i=0;i<shared_shader::predefines_count;++i)
            {
                if(strcmp(semantics,predefined_semantics[i])==0)
                {
                    desc.predefines[i].name=name;
                    desc.predefines[i].transform=transform_from_string(parser.getSectionOption(section_idx));

                    if(i==shared_shader::bones_pos_tex || i==shared_shader::bones_pos_tr_tex)
                    {
                        skeleton_blit.init();
                        res.texture_buffers.allocate();
                        res.texture_buffers->skeleton_pos_max_count=int(parser.getSectionValueVec4(section_idx).x);
                        res.texture_buffers->skeleton_pos_texture.buildTexture(0,res.texture_buffers->skeleton_pos_max_count,1,RoxRender::RoxTexture::COLOR_RGB32F,1);
                        res.texture_buffers->skeleton_pos_texture.setFilter(RoxRender::RoxTexture::FILTER_NEAREST,
                                                                             RoxRender::RoxTexture::FILTER_NEAREST,RoxRender::RoxTexture::FILTER_NEAREST);
                    }
                    else if(i==shared_shader::bones_rot_tex)
                    {
                        skeleton_blit.init();
                        res.texture_buffers.allocate();
                        res.texture_buffers->skeleton_rot_max_count=int(parser.getSectionValueVec4(section_idx).x);
                        res.texture_buffers->skeleton_rot_texture.buildTexture(0,res.texture_buffers->skeleton_rot_max_count,1,RoxRender::RoxTexture::COLOR_RGBA32F,1);
                        res.texture_buffers->skeleton_rot_texture.setFilter(RoxRender::RoxTexture::FILTER_NEAREST,
                                                                             RoxRender::RoxTexture::FILTER_NEAREST,RoxRender::RoxTexture::FILTER_NEAREST);
                    }
                    break;
                }
            }
        }
        else if(parser.isSectionType(section_idx,"uniform"))
        {
            const char *name=parser.getSectionName(section_idx,0);
            const char *semantics=parser.getSectionName(section_idx,1);
            if(!name || !semantics)
            {
                log()<<"unable to load RoxShader "<<name<<": invalid uniform syntax\n";
                return false;
            }

            desc.uniforms[semantics]=name;
            res.uniforms.resize(res.uniforms.size()+1);
            res.uniforms.back().name=semantics;
            res.uniforms.back().transform=transform_from_string(parser.getSectionOption(section_idx));
            res.uniforms.back().default_value=parser.getSectionValueVec4(section_idx);
        }
        else if(parser.isSectionType(section_idx,"procedural"))
        {
            const char *name=parser.getSectionName(section_idx,0);
            RoxFormats::RoxMathExprParser p;
            if(!name || !name[0] || !p.parse(parser.getSectionValue(section_idx)))
            {
                log()<<"unable to load RoxShader "<<name<<": invalid procedural\n";
                return false;
            }

            desc.procedural.push_back(std::make_pair(name,p));
        }
        else
            log()<<"scene RoxShader load warning: unsupported RoxShader tag "<<parser.getSectionType(section_idx)<<" in "<<name<<"\n";
    }

    if(include)
        return true;

    if(desc.VERTEX.empty())
    {
        log()<<"scene RoxShader load error: empty VERTEX RoxShader in "<<name<<"\n";
        return false;
    }

    if(desc.PIXEL.empty())
    {
        log()<<"scene RoxShader load error: empty PIXEL RoxShader in "<<name<<"\n";
        return false;
    }

    //log()<<"VERTEX <"<<res.VERTEX.c_str()<<">\n";
    //log()<<"PIXEL <"<<res.PIXEL.c_str()<<">\n";

    if(!res.shdr.addProgram(RoxRender::RoxShader::VERTEX,desc.VERTEX.c_str()))
        return false;

    if(!res.shdr.addProgram(RoxRender::RoxShader::PIXEL,desc.PIXEL.c_str()))
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
            res.predefines.back().location=res.shdr.getSamplerLayer(p.name.c_str());
            continue;
        }

        res.predefines.back().transform=p.transform;
        res.predefines.back().location=res.shdr.findUniform(p.name.c_str());
    }

    for(int i=0;i<(int)res.uniforms.size();++i)
    {
        const int l=res.uniforms[i].location=res.shdr.findUniform(desc.uniforms[res.uniforms[i].name].c_str());
        const RoxMath::Vector4 &v=res.uniforms[i].default_value;
        res.shdr.setUniform(l,v.x,v.y,v.z,v.w);
    }

    for(shader_description::strings_map::const_iterator it=desc.samplers.begin();
        it!=desc.samplers.end();++it)
    {
        int layer=res.shdr.getSamplerLayer(it->second.c_str());
        if(layer<0)
            continue;

        if(layer>=(int)res.samplers.size())
            res.samplers.resize(layer+1);

        res.samplers[layer]=it->first;
    }

    for(int i=0;i<(int)desc.procedural.size();++i)
    {
        const int idx=res.shdr.findUniform(desc.procedural[i].first.c_str());
        if(idx<0)
            continue;

        const int count=(int)res.shdr.getUniformArraySize(idx);
        RoxFormats::RoxMathExprParser &p=desc.procedural[i].second;
        p.setVar("count",(float)count);

        std::vector<RoxMath::Vector4> values(count);
        for(int j=0;j<count;++j)
        {
            p.setVar("i",(float)j);
            values[j]=p.calculateVec4();
        }

        res.shdr.setUniform4Array(idx,&values[0].x,count);
    }

    return true;
}

bool RoxShader::load_nya_shader(shared_shader &res,resource_data &data,const char* name)
{
    shader_description desc;
    return load_nya_shader_internal(res,desc,data,name,false);
}

bool RoxShader::load(const char *name)
{
    shader_internal::default_load_function(load_nya_shader);
    m_internal.reset_skeleton();
    return m_internal.load(name);
}

void RoxShader::unload()
{
    m_internal.unload();
    if(skeleton_blit.isValid() && !m_internal.get_shared_resources().getFirstResource().isValid())
        skeleton_blit.release();
}

void RoxShader::create(const shared_shader &res)
{
    m_internal.reset_skeleton();
    m_internal.create(res);
}

bool RoxShader::build(const char *code_text)
{
    if(!code_text)
        return false;

    shared_shader s;
    RoxMemory::RoxTmpBufferRef data(strlen(code_text));
    data.copyFrom(code_text,data.getSize());
    const bool result=load_nya_shader(s,data,"");
    data.free();
    create(s);
    return result;
}

void shader_internal::set() const
{
    if(!m_shared.isValid())
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
                    const RoxMath::Vector3 v=transform::get().inverse_transform(get_camera().get_pos());
                    m_shared->shdr.setUniform(p.location,v.x,v.y,v.z);
                }
                else if(p.transform==shared_shader::local_rot)
                {
                    const RoxMath::Vector3 v=transform::get().inverse_rot(get_camera().get_pos());
                    m_shared->shdr.setUniform(p.location,v.x,v.y,v.z);
                }
                else if(p.transform==shared_shader::local_rot_scale)
                {
                    const RoxMath::Vector3 v=transform::get().inverse_rot_scale(get_camera().get_pos());
                    m_shared->shdr.setUniform(p.location,v.x,v.y,v.z);
                }
                else
                {
                    const RoxMath::Vector3 v=get_camera().get_pos();
                    m_shared->shdr.setUniform(p.location,v.x,v.y,v.z);
                }
            }
            break;

            case shared_shader::camera_dir:
            {
                if(p.transform==shared_shader::local_rot || p.transform==shared_shader::local)
                {
                    const RoxMath::Vector3 v=transform::get().inverse_rot(get_camera().get_dir());
                    m_shared->shdr.setUniform(p.location,v.x,v.y,v.z);
                }
                else
                {
                    const RoxMath::Vector3 v=get_camera().get_dir();
                    m_shared->shdr.setUniform(p.location,v.x,v.y,v.z);
                }
            }
            break;

            case shared_shader::camera_rot:
            {
                if(p.transform==shared_shader::local_rot || p.transform==shared_shader::local)
                {
                    const RoxMath::Quaternion v=transform::get().inverse_transform(get_camera().get_rot());
                    m_shared->shdr.setUniform(p.location,v.v.x,v.v.y,v.v.z,v.w);
                }
                else
                {
                    const RoxMath::Quaternion v=get_camera().get_rot();
                    m_shared->shdr.setUniform(p.location,v.v.x,v.v.y,v.v.z,v.w);
                }
            }
            break;

            case shared_shader::bones_pos:
            {
                if(m_skeleton && m_shared->last_skeleton_pos!=m_skeleton)
                {
                    m_shared->shdr.setUniform3Array(p.location,m_skeleton->getPosBuffer(),m_skeleton->getBonesCount());
                    m_shared->last_skeleton_pos=m_skeleton;
                }
            }
            break;

            case shared_shader::bones_pos_tr:
            {
                if(m_skeleton && m_shared->last_skeleton_pos!=m_skeleton)
                {
                    RoxMemory::RoxTmpBufferScoped tmp(m_skeleton->getBonesCount()*3*4);
                    RoxMath::Vector3 *pos=(RoxMath::Vector3 *)tmp.getData();

                    if(m_skeleton->hasOriginalRot())
                    {
                        for(int i=0;i<m_skeleton->getBonesCount();++i)
                        {
                            pos[i]=m_skeleton->getBonePos(i)
                                  +(m_skeleton->getBoneRot(i)*RoxMath::Quaternion::invert(m_skeleton->getBoneOriginalRot(i)))
                                  .rotate(-m_skeleton->getBoneOriginalPos(i));
                        }
                    }
                    else
                    {
                        for(int i=0;i<m_skeleton->getBonesCount();++i)
                        {
                            pos[i]=m_skeleton->getBonePos(i)
                                  +m_skeleton->getBoneRot(i).rotate(-m_skeleton->getBoneOriginalPos(i));
                        }
                    }

                    m_shared->shdr.setUniform3Array(p.location,(float *)tmp.getData(),m_skeleton->getBonesCount());
                    m_shared->last_skeleton_pos=m_skeleton;
                }
            }
            break;

            case shared_shader::bones_rot:
            {
                if(m_skeleton && m_shared->last_skeleton_rot!=m_skeleton)
                {
                    m_shared->shdr.setUniform4Array(p.location,m_skeleton->getRotBuffer(),m_skeleton->getBonesCount());
                    m_shared->last_skeleton_rot=m_skeleton;
                }
            }
            break;

            case shared_shader::bones_rot_tr:
            {
                if(m_skeleton && m_shared->last_skeleton_rot!=m_skeleton)
                {
                    RoxMemory::RoxTmpBufferScoped  tmp(m_skeleton->getBonesCount()*4*4);
                    RoxMath::Quaternion *rot=(RoxMath::Quaternion *)tmp.getData();
                    for(int i=0;i<m_skeleton->getBonesCount();++i)
                        rot[i]=m_skeleton->getBoneRot(i)*RoxMath::Quaternion::invert(m_skeleton->getBoneOriginalRot(i));

                    m_shared->shdr.setUniform4Array(p.location,(float *)tmp.getData(),m_skeleton->getBonesCount());
                    m_shared->last_skeleton_rot=m_skeleton;
                }
            }
            break;

            case shared_shader::bones_pos_tex:
            {
                if(!m_shared->texture_buffers.isValid())
                    m_shared->texture_buffers.allocate();

                if(m_skeleton && m_shared->texture_buffers->last_skeleton_pos_texture!=m_skeleton && m_skeleton->getBonesCount()>0)
                {
                    skeleton_blit.blit(m_shared->texture_buffers->skeleton_pos_texture,m_skeleton->getPosBuffer(),m_skeleton->getBonesCount(),3);
                    m_shared->texture_buffers->last_skeleton_pos_texture=m_skeleton;
                }

                m_shared->texture_buffers->skeleton_pos_texture.bind(p.location);
            }
            break;

            case shared_shader::bones_pos_tr_tex:
            {
                if(!m_shared->texture_buffers.isValid())
                    m_shared->texture_buffers.allocate();

                //ToDo: if(m_skeleton->has_original_rot())

                if(m_skeleton && m_shared->texture_buffers->last_skeleton_pos_texture!=m_skeleton && m_skeleton->getBonesCount()>0)
                {
                    RoxMemory::RoxTmpBufferScoped  tmp(m_skeleton->getBonesCount()*3*4);
                    RoxMath::Vector3 *pos=(RoxMath::Vector3 *)tmp.getData();
                    for(int i=0;i<m_skeleton->getBonesCount();++i)
                        pos[i]=m_skeleton->getBonePos(i)+m_skeleton->getBoneRot(i).rotate(-m_skeleton->getBoneOriginalPos(i));

                    skeleton_blit.blit(m_shared->texture_buffers->skeleton_pos_texture,(float *)tmp.getData(),m_skeleton->getBonesCount(),3);
                    m_shared->texture_buffers->last_skeleton_pos_texture=m_skeleton;
                }

                m_shared->texture_buffers->skeleton_pos_texture.bind(p.location);
            }
            break;

            case shared_shader::bones_rot_tex:
            {
                if(!m_shared->texture_buffers.isValid())
                    m_shared->texture_buffers.allocate();

                if(m_skeleton && m_shared->texture_buffers->last_skeleton_rot_texture!=m_skeleton && m_skeleton->getBonesCount()>0)
                {
                    skeleton_blit.blit(m_shared->texture_buffers->skeleton_rot_texture,m_skeleton->getRotBuffer(),m_skeleton->getBonesCount(),4);
                    m_shared->texture_buffers->last_skeleton_rot_texture=m_skeleton;
                }

                m_shared->texture_buffers->skeleton_rot_texture.bind(p.location);
            }
            break;

            case shared_shader::viewport:
            {
                RoxRender::Rectangle r=RoxRender::getViewport();
                m_shared->shdr.setUniform(p.location,float(r.x),float(r.y),float(r.width),float(r.height));
            }
            break;

            case shared_shader::model_pos:
            {
                const RoxMath::Vector3 v=transform::get().get_pos();
                m_shared->shdr.setUniform(p.location,v.x,v.y,v.z);
            }
            break;

            case shared_shader::model_rot:
            {
                const RoxMath::Quaternion v=transform::get().get_rot();
                m_shared->shdr.setUniform(p.location,v.v.x,v.v.y,v.v.z,v.w);
            }
            break;

            case shared_shader::model_scale:
            {
                const RoxMath::Vector3 v=transform::get().get_scale();
                m_shared->shdr.setUniform(p.location,v.x,v.y,v.z);
            }
            break;

            case shared_shader::predefines_count: break;
        }
    }

    m_shared->shdr.bind();
}

int shader_internal::get_texture_slot(const char *semantics) const
{
    if(!semantics || !m_shared.isValid())
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
    if(!m_shared.isValid() || slot<0 || slot>=(int)m_shared->samplers.size())
        return 0;

    return m_shared->samplers[slot].c_str();
}

int shader_internal::get_texture_slots_count() const
{
    if(!m_shared.isValid())
        return 0;

    return (int)m_shared->samplers.size();
}

int shader_internal::get_uniform_idx(const char *name) const
{
    if(!m_shared.isValid() || !name)
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
    if(!m_shared.isValid() || idx<0 || idx >=(int)m_shared->uniforms.size())
        return RoxMemory::invalidObject<shared_shader::uniform>();

    return m_shared->uniforms[idx];
}

RoxRender::RoxShader::UNIFORM_TYPE shader_internal::get_uniform_type(int idx) const
{
    if(!m_shared.isValid())
        return RoxRender::RoxShader::UNIFORM_NOT_FOUND;

    return m_shared->shdr.getUniformType(get_uniform(idx).location);
}

unsigned int shader_internal::get_uniform_array_size(int idx) const
{
    if(!m_shared.isValid())
        return 0;

    return m_shared->shdr.getUniformArraySize(get_uniform(idx).location);
}

void shader_internal::set_uniform_value(int idx,float f0,float f1,float f2,float f3) const
{
    if(!m_shared.isValid() || idx<0 || idx >=(int)m_shared->uniforms.size())
        return;

    if(m_shared->uniforms[idx].location<0)
        return;

    if(m_shared->uniforms[idx].transform==shared_shader::local)
    {
        RoxMath::Vector3 v=transform::get().inverse_transform(RoxMath::Vector3(f0,f1,f2));
        m_shared->shdr.setUniform(m_shared->uniforms[idx].location,v.x,v.y,v.z,f3);
    }
    else if(m_shared->uniforms[idx].transform==shared_shader::local_rot)
    {
        RoxMath::Vector3 v=transform::get().inverse_rot(RoxMath::Vector3(f0,f1,f2));
        m_shared->shdr.setUniform(m_shared->uniforms[idx].location,v.x,v.y,v.z,f3);
    }
    else if(m_shared->uniforms[idx].transform==shared_shader::local_rot_scale)
    {
        RoxMath::Vector3 v=transform::get().inverse_rot_scale(RoxMath::Vector3(f0,f1,f2));
        m_shared->shdr.setUniform(m_shared->uniforms[idx].location,v.x,v.y,v.z,f3);
    }
    else
        m_shared->shdr.setUniform(m_shared->uniforms[idx].location,f0,f1,f2,f3);
}

void shader_internal::set_uniform4_array(int idx,const float *array,unsigned int count) const
{
    if(!m_shared.isValid() || idx<0 || idx >=(int)m_shared->uniforms.size())
        return;

    m_shared->shdr.setUniform4Array(m_shared->uniforms[idx].location,array,count);
}

int shader_internal::get_uniforms_count() const
{
    if(!m_shared.isValid())
        return 0;

    return (int)m_shared->uniforms.size();
}

const RoxRender::RoxSkeleton *shader_internal::m_skeleton=0;

void shader_internal::skeleton_changed(const RoxRender::RoxSkeleton *RoxSkeleton) const
{
    if(!m_shared.isValid())
        return;

    if(RoxSkeleton==m_shared->last_skeleton_pos)
        m_shared->last_skeleton_pos=0;

    if(RoxSkeleton==m_shared->last_skeleton_rot)
        m_shared->last_skeleton_rot=0;

    if(m_shared->texture_buffers.isValid())
    {
        if(RoxSkeleton==m_shared->texture_buffers->last_skeleton_pos_texture)
            m_shared->texture_buffers->last_skeleton_pos_texture=0;

        if(RoxSkeleton==m_shared->texture_buffers->last_skeleton_rot_texture)
            m_shared->texture_buffers->last_skeleton_rot_texture=0;
    }
}

}
