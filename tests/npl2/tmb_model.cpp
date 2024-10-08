//nya-engine (C) nyan.developer@gmail.com released under the MIT license (see LICENSE)

#include "tmb_model.h"
#include "tsb_anim.h"
#include "memory/tmp_buffer.h"
#include "render/render.h"

#include "string.h"
#include <vector>

namespace
{
    struct vertex
    {
        float pos[3];
        float nor[3];
        float tc[2];
        float bone_idx[4];
        float bone_weight[4];
        float color[4];
    };
}

bool tmb_model::load(nya_resources::resource_data *model_res)
{
    release();

    if(!model_res)
    {
        nya_log::log()<<"Unable to load model: invalid data\n";
        return false;
    }
    
    const size_t size=model_res->get_size();

    nya_memory::tmp_buffer_scoped model_data(model_res->get_size());
    model_res->read_all(model_data.get_data());
    model_res->release();

    const char *magic=(const char*)model_data.get_data();
    if(strncmp(magic,"TMB0",4)!=0)
    {
        nya_log::log()<<"Warning: invalid TMB0 magic\n";
        //return false;
    }

    typedef unsigned int uint;
    typedef unsigned short ushort;
    typedef unsigned char uchar;

    uint offset=4;

    const uint tex_count=*(uint*)model_data.get_data(offset);
    offset+=4;

    m_textures.resize(tex_count);
    for(int i=0;i<tex_count;++i)
    {
        struct tmb_tex_header
        {
            char name[32];
            ushort width;
            ushort height;
        };

        tmb_tex_header *header=(tmb_tex_header*)model_data.get_data(offset);
        offset+=sizeof(tmb_tex_header);
        m_textures[i].build_texture(model_data.get_data(offset),header->width,
                                    header->height,nya_render::texture::color_bgra);

        offset+=header->width*header->height*4;
        
        if(offset>=size)
            return false;
    }

    const uint tmb_mat_count=*(uint*)model_data.get_data(offset);
    offset+=4;

    if(offset>=size)
        return false;

    struct tmb_material
    {
        float ambient[4];
        float diffuse[4];
        float specular[4];
        float emission[4];
        float shininess;
        uint tex_idx;
    };

    const tmb_material *tmb_materials=(tmb_material*)model_data.get_data(offset);
    offset+=tmb_mat_count*sizeof(tmb_material);
    
    if(offset>=size)
        return false;

    const uint group_count=*(uint*)model_data.get_data(offset);
    offset+=4;
    
    if(offset>=size)
        return false;

    m_group_names.resize(group_count);

    static std::vector<vertex> vertices;

    vertices.clear();
    m_materials.clear();
    m_vbo.release();

    uint verts_offset=0;
    int mat_offset=0;
    for(int i=0;i<group_count;++i)
    {
        struct tmb_group_header
        {
            char name[32];
            float matrix[4][4];
            uint faces_count;
            uint cull_face;
            uint mat_bind_count;
        };

        const tmb_group_header *header=(tmb_group_header*)model_data.get_data(offset);
        offset+=sizeof(tmb_group_header);
        
        if(offset>=size)
            return false;

        m_group_names[i].assign(header->name);

        //nya_log::get_log()<<"Group "<<i<<" "<<header->name<<" unknown "<<header->unknown<<"\n";

        const uint mat_count=header->mat_bind_count;
        m_materials.resize(mat_offset+mat_count);

        const uint verts_count=header->faces_count*3;
        vertices.resize(verts_offset+verts_count);

        struct tmb_mat_bind
        {
            uint mat_idx;
            int vert_offset;
            int vert_count;
        };

        const tmb_mat_bind *tmb_mat_binds=(tmb_mat_bind*)model_data.get_data(offset);
        offset+=header->mat_bind_count*sizeof(tmb_mat_bind);

        if(offset>=size)
            return false;

        for(int j=0;j<header->mat_bind_count;++j)
        {
            material &to=m_materials[mat_offset];
            const tmb_mat_bind &from_bind=tmb_mat_binds[j];
            if(from_bind.mat_idx>=tmb_mat_count)
                continue;

            const tmb_material &from_mat=tmb_materials[from_bind.mat_idx];

            if(from_bind.vert_count<0 || from_bind.vert_offset<0)
                continue;

            to.group=i;
            to.vert_offset=from_bind.vert_offset+verts_offset;
            to.vert_count=from_bind.vert_count*3;
            to.tex_idx=from_mat.tex_idx;
            to.cull_face=header->cull_face>0;

            ++mat_offset;
        }

        m_materials.resize(mat_offset);

        struct tmb_vertex
        {
            float pos[3];
            float weights[3]; //forth weight is (1.0 - other weights)
            uchar bone_idx[4];
            float normal[3];
            uchar color[4];
            float tc[2];
        };

        const tmb_vertex *tmb_vertices=(tmb_vertex*)model_data.get_data(offset);
        offset+=verts_count*sizeof(tmb_vertex);
        
        if(offset>=size)
            return false;

        for(int j=0;j<verts_count;++j)
        {
            const tmb_vertex &from=tmb_vertices[j];
            vertex &to=vertices[j+verts_offset];

            for(int k=0;k<3;++k)
            {
                to.pos[k]=header->matrix[0][k]*from.pos[0]
                         +header->matrix[1][k]*from.pos[1]
                         +header->matrix[2][k]*from.pos[2]
                         +header->matrix[3][k];

                to.nor[k]=from.normal[k];

                to.color[k]=from.color[k]/255.0f;

                if(from.weights[k]>0.01f)
                {
                    to.bone_weight[k]=from.weights[k];
                    to.bone_idx[k]=(float)from.bone_idx[k];
                }
                else
                {
                    to.bone_idx[k]=0;
                    to.bone_weight[k]=0;
                }
            }

            to.color[3]=1.0f;//from.color[3]/255.0f;

            const float weight3=1.0f-(from.weights[0]+from.weights[1]+from.weights[2]);
            if(weight3>0.01f)
            {
                to.bone_idx[3]=(float)from.bone_idx[3];
                to.bone_weight[3]=weight3;
            }
            else
            {
                to.bone_idx[3]=0;
                to.bone_weight[3]=0;
            }

            to.tc[0]=from.tc[0];
            to.tc[1]=from.tc[1];
        }

        verts_offset+=verts_count;
    }

    uint bones_count=*(uint*)model_data.get_data(offset);
    offset+=4;
    
    if(offset>=size)
        return false;

    if(bones_count)
    {
        m_bones.resize(bones_count);
        model_data.copy_to(&m_bones[0],bones_count*sizeof(nya_math::mat4),offset);
    }
    else
        m_bones.clear();

    double average_color[3]={0,0,0};
    if(!vertices.empty())
    {
        double inv_count=1.0/vertices.size();
        for(uint i=0;i<vertices.size();++i)
        {
            for(int j=0;j<3;++j)
                average_color[j]+=inv_count*vertices[i].color[j];
        }
    }

    for(int j=0;j<3;++j)
    {
        average_color[j]*=0.6f;
        average_color[j]+=0.4f;
    }

    m_vbo.set_vertex_data(&vertices[0],sizeof(vertex),verts_offset);
    m_vbo.set_normals(3*sizeof(float));
    m_vbo.set_tc(0,6*sizeof(float),2);
    m_vbo.set_tc(1,8*sizeof(float),4);
    m_vbo.set_tc(2,12*sizeof(float),4);
    m_vbo.set_tc(3,16*sizeof(float),4);

    offset+=bones_count*sizeof(nya_math::mat4);
    uint locators_count=*(uint*)model_data.get_data(offset);
    offset+=4;
    //nya_log::get_log()<<"locators: "<<locators_count<<"offset: "<<offset<<"\n";
    
    if(offset>=size)
        return false;

    struct tmb_locator
    {
        char unknown[4];
        float pos[3];
        float ang[3];
        float scale[3];
    };

    m_locators.resize(locators_count);

    for(int i=0;i<locators_count;++i)
    {
        if(offset>=size)
            return false;

        tmb_locator *from=(tmb_locator*)model_data.get_data(offset);
        locator &to=m_locators[i];

        for(int j=0;j<3;++j)
        {
            to.pos[j]=from->pos[j];
            to.ang[j]=from->ang[j];
            to.scale[j]=from->scale[j];

            to.color[j]=average_color[j];
        }

        offset+=40;
    }

    //nya_log::get_log()<<(const char*)model_data.get_data(offset);

    /*
    for(unsigned int i=0;i<m_materials.size();++i)
    {
        material & mat = m_materials[i];

        int max_bones=0;

        for(int j=mat.vert_offset;j<mat.vert_offset+mat.vert_count;++j)
        {
            if(max_bones==4)
                break;

            for(int k=max_bones;k<4;++k)
                if(vertices[j].bone_weight[k]>0)
                    max_bones=k+1;
        }

        nya_log::get_log()<<"max_bones: "<<max_bones<<"in group "<<i<<"vertices "<<mat.vert_count<<"\n";

        //glColor4fv(mat.color);
        //m_vbo.draw(mat.vert_offset,mat.vert_count);
    }
    */

    return true;
}

void tmb_model::draw(bool use_materials,bool cull_face,int group) const
{
    m_vbo.bind();
    if(!use_materials)
    {
        m_vbo.draw();
        m_vbo.unbind();
        return;
    }

    for(unsigned int i=0;i<m_materials.size();++i)
    {
        const material & mat = m_materials[i];

        if(group>=0 && group!=mat.group)
            continue;

        if(!cull_face)
            nya_render::cull_face::enable(nya_render::cull_face::cw);
        else if(mat.cull_face)
            nya_render::cull_face::enable(nya_render::cull_face::ccw);
        else
            nya_render::cull_face::disable();

        m_textures[mat.tex_idx].bind(0);

        //glColor4fv(mat.color);
        m_vbo.draw(mat.vert_offset,mat.vert_count);
    }
    m_vbo.unbind();
    nya_render::cull_face::disable();
}

void tmb_model::release()
{
    m_vbo.release();
    for(unsigned int i=0;i<m_textures.size();++i)
        m_textures[i].release();

    m_textures.clear();
    m_materials.clear();
    m_group_names.clear();
    m_locators.clear();
    m_bones.clear();
}

bool shared_models_manager::fill_resource(const char *name,tmb_model &res)
{
    if(!name)
    {
        nya_resources::log()<<"Unable to access model: invalid name\n";
        return false;
    }

    nya_resources::resource_data *data = nya_resources::get_resources_provider().access(name);
    return res.load(data);
}

bool shared_models_manager::release_resource(tmb_model &res)
{
    //nya_resources::get_log()<<"released\n";

    res.release();
    return true;
}

shared_models_manager &get_shared_models()
{
    static shared_models_manager manager;
    return manager;
}
