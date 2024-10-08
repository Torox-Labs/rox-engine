//nya-engine (C) nyan.developer@gmail.com released under the MIT license (see LICENSE)

#include "mmd_mesh.h"
#include "scene/camera.h"
#include <float.h>

bool mmd_mesh::load(const char *name)
{
    unload();

    nya_scene::mesh::register_load_function(pmx_loader::load,false);
    nya_scene::mesh::register_load_function(pmd_loader::load,false);

    return mesh::load(name) && init();
}

bool mmd_mesh::init()
{
    m_pmd_data=pmd_loader::get_additional_data(*this);
    m_pmx_data=pmx_loader::get_additional_data(*this);

    if(!is_mmd())
        return true;

    m_skining.create();

    m_vmorphs.create();
    m_morphs_count.create();

    if(m_pmd_data)
    {
        m_morphs.resize(m_pmd_data->morphs.size());
        m_skining->mat.get_default_pass().set_shader("pmd_skining.nsh");
    }
    else if(m_pmx_data)
    {
        m_morphs.resize(m_pmx_data->morphs.size());
        m_morphs_count->y=float(m_pmx_data->vert_tex_w);
        m_morphs_count->z=float(m_pmx_data->vert_tex_h);
        m_skining->mat.get_default_pass().set_shader("pmx_skining.nsh");
    }

    m_skining->mat.set_texture("bones", m_skining->bones);
    m_skining->mat.set_param_array("morphs", m_vmorphs);

    m_skining->mat.set_texture("morph", get_material(0).get_texture("morph")); //ToDo
    m_skining->mat.set_param("morphs count", m_morphs_count);

    const nya_render::vbo &sh_vbo=internal().get_shared_data()->vbo;
    m_skining->vbo.set_vertex_data(0,(3+3+2)*sizeof(float),sh_vbo.get_verts_count());
    int offset=0;
    m_skining->vbo.set_vertices(0,3); offset+=sizeof(float)*3;
    m_skining->vbo.set_normals(offset); offset+=sizeof(float)*3;
    m_skining->vbo.set_tc(0,offset,2);

    m_skining->buffer.resize(get_skeleton().get_bones_count()*2);

    m_skip_groups.resize(get_groups_count(),false);
    if (m_pmx_data)
    {
        for(int i=0;i<(int)m_pmx_data->materials.size();++i)
        {
            const float eps = 0.001f;
            m_skip_groups[i]=m_pmx_data->materials[i].params.diffuse.w<eps;

            const pmx_loader::pmx_edge_params &edge=m_pmx_data->materials[i].edge_params;
            const int edge_idx=m_pmx_data->materials[i].edge_group_idx;
            if (edge_idx>=0)
                m_skip_groups[edge_idx]=edge.color.w<eps || edge.width<eps;
        }
    }

    update(0);
    return true;
}

void mmd_mesh::unload()
{
    mesh::unload();
    m_pmd_data=0;
    m_pmx_data=0;
    m_morphs.clear();
}

void mmd_mesh::set_anim(const nya_scene::animation &anim,int layer,bool lerp)
{
    mmd_mesh::set_anim(nya_scene::animation_proxy(anim),layer,lerp);
}

void mmd_mesh::set_anim(const nya_scene::animation_proxy &anim,int layer,bool lerp)
{
    mesh::set_anim(anim,layer,lerp);
    if (!anim.is_valid())
        return;

    for(int i=0;i<m_morphs.size();++i)
        m_morphs[i].last_value=-1.0f;

    int idx=-1;
    for(int i=0;i<int(m_anims.size());++i)
    {
        if(m_anims[i].layer==layer)
        {
            idx=i;
            break;
        }
    }

    if(idx<0)
    {
        idx=int(m_anims.size());
        m_anims.resize(idx+1);
        m_anims[idx].layer=layer;
    }

    m_anims[idx].lerp = lerp;
    m_anims[idx].curves_map.clear();
    m_anims[idx].curves_map.resize(m_morphs.size(), -1);
    m_anims[idx].sh = anim->get_shared_data().operator->();
    if(!anim->get_shared_data().is_valid())
        return;

    if(m_pmd_data)
    {
        for(int i=0;i<int(m_morphs.size());++i)
            m_anims[idx].curves_map[i]=anim->get_shared_data()->anim.get_curve_idx(m_pmd_data->morphs[i].name.c_str());
    }
    else if(m_pmx_data)
    {
        for(int i=0;i<int(m_morphs.size());++i)
            m_anims[idx].curves_map[i]=anim->get_shared_data()->anim.get_curve_idx(m_pmx_data->morphs[i].name.c_str());
    }
}

void mmd_mesh::set_morph(int idx,float value,bool overriden)
{
    if(idx<0 || idx>=int(m_morphs.size()))
        return;

    if(value<=0.001f)
        value=0.0f;

    m_morphs[idx].value=value;
    m_morphs[idx].overriden=overriden;
}

int mmd_mesh::find_morph(const char *name) const
{
    if(!name)
        return -1;

    for(int i=0;i<get_morphs_count();++i)
    {
        if(strcmp(get_morph_name(i),name)==0)
            return i;
    }

    return -1;
}

void mmd_mesh::update(unsigned int dt)
{
    mesh::update(dt);

    if(!is_mmd())
        return;

    if(!m_anims.empty())
    {
        for(int i=0;i<(int)m_morphs.size();++i)
        {
            if(!m_morphs[i].overriden)
                m_morphs[i].value=0;
        }

        for(int i=0;i<m_anims.size();++i)
        {
            const nya_scene::animation_proxy &ap=get_anim(m_anims[i].layer);
            if(!ap.is_valid() || !ap->get_shared_data().is_valid())
                continue;

            if (ap->get_shared_data().operator->() != m_anims[i].sh)
                set_anim(ap, m_anims[i].layer, m_anims[i].lerp);

            const nya_render::animation &a=ap->get_shared_data()->anim;
            const int time=get_anim_time(m_anims[i].layer);
            const float weight=ap->get_weight();

            for(int j=0;j<m_anims[i].curves_map.size();++j)
            {
                if(m_morphs[j].overriden)
                    continue;

                m_morphs[j].value+=a.get_curve(m_anims[i].curves_map[j],time)*weight;
            }
        }
    }

    if(m_pmx_data)
    {
        //update group morphs first
        for(auto &m: m_morphs)
            m.group_value=0.0f;

        for(int i=0;i<(int)m_pmx_data->morphs.size();++i)
        {
            const pmx_loader::additional_data::morph &m=m_pmx_data->morphs[i];
            if(m.type!=pmx_loader::morph_type_group)
                continue;

            //if(m_morphs[i].value==m_morphs[i].last_value)
            //    continue;

            const pmx_loader::additional_data::group_morph &gm=m_pmx_data->group_morphs[m.idx];
            for(int j=0;j<(int)gm.morphs.size();++j)
            {
                float &v=m_morphs[gm.morphs[j].first].group_value;
                v=nya_math::max(v,m_morphs[i].value*gm.morphs[j].second);
            }

            m_morphs[i].last_value=m_morphs[i].value;
        }

        //update material morphs before last_value updated
        update_material_morphs();
    }

    if(m_vmorphs.is_valid())
        m_vmorphs->set_count(0);

    for(int i=0;i<m_morphs.size();++i)
    {
        const float value=nya_math::max(m_morphs[i].value,m_morphs[i].group_value);

        if(m_pmd_data)
        {
            //ToDo

            /*            const pmd_morph_data::morph &m=m_pmd_data->morphs[i];
             for(int j=0;j<int(m.verts.size());++j)
             m_vertex_data[m.verts[j].idx].pos+=m.verts[j].pos*m_morphs[i].value;
             */
        }
        else if(m_pmx_data)
        {
            const pmx_loader::additional_data::morph &m=m_pmx_data->morphs[i];
            switch(m.type)
            {
                case pmx_loader::morph_type_vertex:
                {
                    if(value<0.001f)
                        continue;

                    const int idx=m_vmorphs->get_count();
                    m_vmorphs->set_count(idx+1);
                    m_vmorphs->set(idx,value,float(m.idx*m_pmx_data->mvert_count));
                    break;
                }

                case pmx_loader::morph_type_bone:
                {
                    if(value==m_morphs[i].last_value)
                        continue;

                    const pmx_loader::additional_data::bone_morph &bm=m_pmx_data->bone_morphs[m.idx];
                    for(int j=0;j<int(bm.bones.size());++j)
                    {
                        set_bone_pos(bm.bones[j].idx,bm.bones[j].pos*m_morphs[i].value,true);
                        nya_math::quat rot=bm.bones[j].rot;
                        rot.apply_weight(m_morphs[i].value);
                        set_bone_rot(bm.bones[j].idx,rot,true);
                    }
                    break;
                }

                default: break;
            }
        }

        m_morphs[i].last_value=value;
    }
    
    const int vmorphs_count=m_vmorphs->get_count();

    if(m_pmx_data)
    {
        for(int i=0;i<m_morphs.size();++i)
        {
            const pmx_loader::additional_data::morph &m=m_pmx_data->morphs[i];
            if(m.type!=pmx_loader::morph_type_uv)
                continue;

            const float value=nya_math::max(m_morphs[i].value,m_morphs[i].group_value);
            if(value<0.001f)
                continue;

            const int idx=m_vmorphs->get_count();
            m_vmorphs->set_count(idx+1);
            m_vmorphs->set(idx,value,float((m_pmx_data->vert_morphs.size() + m.idx)*m_pmx_data->mvert_count));
            break;
        }
    }

    if(m_morphs_count.is_valid())
    {
        m_morphs_count->x=float(vmorphs_count);
        m_morphs_count->w=float(m_vmorphs->get_count());
    }

    m_update_skining=true;
}

void mmd_mesh::update_material_morphs()
{
    bool need_update=false;
    for(int i=0;i<(int)m_morphs.size();++i)
    {
        const float value=nya_math::max(m_morphs[i].value,m_morphs[i].group_value);
        if(m_pmx_data->morphs[i].type==pmx_loader::morph_type_material && value!=m_morphs[i].last_value)
        {
            need_update=true;
            break;
        }
    }

    if(!need_update)
        return;

    pmx_loader::pmx_material_params params;
    pmx_loader::pmx_edge_params edge;

    for(int i=0;i<(int)m_pmx_data->materials.size();++i)
    {
        bool set=false;

        for(int j=0;j<(int)m_morphs.size();++j)
        {
            const float value=nya_math::max(m_morphs[j].value,m_morphs[j].group_value);

            const pmx_loader::additional_data::morph &m=m_pmx_data->morphs[j];
            if(m.type!=pmx_loader::morph_type_material)
                continue;

            if(value<=0 && value==m_morphs[j].last_value)
                continue;

            const pmx_loader::additional_data::mat_morph &mm=m_pmx_data->mat_morphs[m.idx];
            for(int k=0;k<(int)mm.mats.size();++k)
            {
                if(mm.mats[k].mat_idx!=i)
                    continue;

                if(!set)
                {
                    params=m_pmx_data->materials[i].params;
                    edge=m_pmx_data->materials[i].edge_params;
                    set=true;
                }

                const pmx_loader::additional_data::morph_mat &m=mm.mats[k];
                if(m.op==pmx_loader::additional_data::morph_mat::op_add)
                {
                    params.ambient+=m.params.ambient*value;
                    params.diffuse+=m.params.diffuse*value;
                    params.specular+=m.params.specular*value;
                    params.shininess+=m.params.shininess*value;

                    edge.color+=m.edge_params.color*value;
                    edge.width+=m.edge_params.width*value;
                }
                else if(m.op==pmx_loader::additional_data::morph_mat::op_mult)
                {
                    params.ambient*=nya_math::vec3::lerp(nya_math::vec3::one(),m.params.ambient,value);
                    params.diffuse*=nya_math::vec4::lerp(nya_math::vec4::one(),m.params.diffuse,value);
                    params.specular*=nya_math::vec3::lerp(nya_math::vec3::one(),m.params.specular,value);
                    params.shininess*=nya_math::lerp(1,m.params.shininess,value);

                    edge.color*=nya_math::vec4::lerp(nya_math::vec4::one(),m.edge_params.color,value);
                    edge.width*=nya_math::lerp(1.0f,m.edge_params.width,value);
                }
            }
        }

        if(!set)
            continue;

        const float eps = 0.001f;
        m_skip_groups[i]=params.diffuse.w<eps;

        nya_scene::material &mat=modify_material(i);
        mat.set_param("amb k",params.ambient,1.0f);
        mat.set_param("diff k",params.diffuse);
        mat.set_param("spec k",params.specular,params.shininess);

        const int edge_idx=m_pmx_data->materials[i].edge_group_idx;
        if (edge_idx>=0)
        {
            nya_scene::material &mat=modify_material(edge_idx);
            mat.set_param("edge offset",edge.width*0.02f,edge.width*0.02f,edge.width*0.02f,0.0f);
            mat.set_param("edge color",edge.color);
            m_skip_groups[edge_idx]=edge.color.w<eps || edge.width<eps;
        }
    }
}

const char *mmd_mesh::get_morph_name(int idx) const
{
    if(idx<0 || idx>=get_morphs_count())
        return 0;

    if (m_pmd_data)
        return m_pmd_data->morphs[idx].name.c_str();

    if (m_pmx_data)
        return m_pmx_data->morphs[idx].name.c_str();

    return 0;
}

void mmd_mesh::update_skining() const
{
    if(!m_update_skining)
        return;

    m_skining->swap();

    const nya_render::skeleton &sk=get_skeleton();
    const int bones_count=sk.get_bones_count();
    const nya_math::vec3 *pos=(nya_math::vec3 *)sk.get_pos_buffer();
    const nya_math::quat *rot=(nya_math::quat *)sk.get_rot_buffer();
    for(int i=0;i<bones_count;++i)
    {
        nya_math::vec3 p=pos[i]+rot[i].rotate(-sk.get_bone_original_pos(i));
        m_skining->buffer[i].set(p,1.0f);
    }
    memcpy(&m_skining->buffer[bones_count],sk.get_rot_buffer(),bones_count*sizeof(nya_math::quat));

    nya_render::texture::filter fmin,fmax,fmip,near=nya_render::texture::filter_nearest;
    const unsigned int aniso=nya_render::texture::get_default_aniso();
    nya_render::texture::get_default_filter(fmin,fmax,fmip);
    nya_render::texture::set_default_filter(near,near,near);
    nya_render::texture::set_default_aniso(0);
    m_skining->bones->build(m_skining->buffer.data(),sk.get_bones_count(),2,nya_render::texture::color_rgba32f);
    nya_render::texture::set_default_filter(fmin,fmax,fmip);
    nya_render::texture::set_default_aniso(aniso);

    const nya_render::vbo &vbo=internal().get_shared_data()->vbo;
    m_skining->mat.internal().set();
    vbo.bind_verts();
    nya_render::vbo::transform_feedback(m_skining->vbo,0,0,vbo.get_verts_count(),nya_render::vbo::points);
    nya_render::vbo::unbind();
    m_skining->mat.internal().unset();
    m_update_skining=false;
}

void mmd_mesh::draw(const char *pass_name) const
{
    for(int i=0;i<get_groups_count();++i)
        draw_group(i,pass_name);
}

void mmd_mesh::draw_group(int group_idx,const char *pass_name) const
{
    if(!is_mmd())
    {
        mesh::draw_group(group_idx,pass_name);
        return;
    }

    const nya_scene::shared_mesh *sh=internal().get_shared_data().operator->();
    if(group_idx<0 || group_idx>=(int)sh->groups.size())
        return;

    if(!m_skip_groups.empty() && m_skip_groups[group_idx])
        return;

    update_skining();

    const nya_scene::shared_mesh::group &g=sh->groups[group_idx];
    nya_scene::transform::set(internal().get_transform());
    const nya_scene::material &m=get_material(group_idx);
    m.internal().set(pass_name);
    m_skining->vbo.bind_verts();
    sh->vbo.bind_indices();
    m_skining->vbo.draw(g.offset,g.count,g.elem_type);
    nya_render::vbo::unbind();
    m.internal().unset();
}

pmd_morph_data::morph_kind mmd_mesh::get_morph_kind(int idx) const
{
    if(idx<0 || idx>=get_morphs_count())
        return pmd_morph_data::morph_base;

    if (m_pmd_data)
        return m_pmd_data->morphs[idx].kind;

    if (m_pmx_data)
        return m_pmx_data->morphs[idx].kind;

    return pmd_morph_data::morph_base;
}

float mmd_mesh::get_morph(int idx) const
{
    if(idx<0 || idx>=get_morphs_count())
        return 0.0f;

    return m_morphs[idx].value;
}
