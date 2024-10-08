//nya-engine (C) nyan.developer@gmail.com released under the MIT license (see LICENSE)

#include "location.h"
#include "formats/text_parser.h"
#include "formats/string_convert.h"

namespace nya_scene
{

bool location::load_text(shared_location &res,resource_data &data,const char* name)
{
    nya_formats::text_parser parser;
    if(!parser.load_from_data((char *)data.get_data(),data.get_size()))
        return false;

    tags local_tags;
    std::string local_mesh_path;
    for(int i=0;i<parser.get_sections_count();++i)
    {
        const char *type=parser.get_section_type(i);
        if(strcmp(type,"@object")==0)
        {
            shared_location::location_mesh m;
            m.tg=local_tags;
            for(int j=0;j<parser.get_subsections_count(i);++j)
            {
                const char *type=parser.get_subsection_type(i,j);
                if(strcmp(type,"mesh")==0)
                    m.name=local_mesh_path+parser.get_subsection_value(i,j);
                else if(strcmp(type,"tags")==0)
                    m.tg.add(tags(parser.get_subsection_value(i,j)));
                else if(strcmp(type,"pos")==0)
                {
                    const nya_math::vec4 v=nya_formats::vec4_from_string(parser.get_subsection_value(i,j));
                    m.tr.set_pos(v.xyz());
                }
                else if(strcmp(type,"rot")==0)
                {
                    const nya_math::vec4 v=nya_formats::vec4_from_string(parser.get_subsection_value(i,j));
                    m.tr.set_rot(v.x,v.y,v.z);
                }
                else if(strcmp(type,"scale")==0)
                {
                    const nya_math::vec4 v=nya_formats::vec4_from_string(parser.get_subsection_value(i,j));
                    m.tr.set_scale(v.x,v.y,v.z);
                }
            }

            res.meshes.push_back(m);
        }
        else if(strcmp(type,"@material_param")==0)
        {
            nya_math::vec4 v=parser.get_section_value_vec4(i);
            const char *opt=parser.get_section_option(i);
            if(strcmp(opt,"normalize")==0)
                v.normalize();

            res.material_params.push_back(std::make_pair(parser.get_section_name(i),v));
        }
        else if(strcmp(type,"@tags")==0)
            local_tags=parser.get_section_value(i);
        else if(strcmp(type,"@mesh_folder")==0)
            local_mesh_path.assign(parser.get_section_value(i));
    }

    return true;
}

bool location::load(const char *name)
{
    default_load_function(load_text);
    unload();

    if(!scene_shared::load(name))
        return false;

    if(!m_shared.is_valid())
        return false;

    m_material_params.resize(m_shared->material_params.size());
    for(size_t j=0;j<m_shared->material_params.size();++j)
    {
        m_material_params[j].first=m_shared->material_params[j].first;
        m_material_params[j].second=material::param_proxy(m_shared->material_params[j].second);
    }

    for(size_t i=0;i<m_shared->meshes.size();++i)
    {
        const shared_location::location_mesh &m=m_shared->meshes[i];
        add_mesh(m.name.c_str(),m.tg,m.tr);
    }

    return true;
}

void location::unload()
{
    m_meshes.clear();
    m_material_params.clear();
    scene_shared::unload();
}

int location::add_mesh(const char *mesh_name,const tags &tg,const transform &tr)
{
    const int mesh_idx=m_meshes.get_count();

    std::vector<const char *> tp(tg.get_count());
    for(int i=0;i<tg.get_count();++i) tp[i]=tg.get(i);
    location_mesh &lm=m_meshes.add(tp.empty()?0:&tp[0],tg.get_count());
    if(mesh_name)
        lm.m.load(mesh_name);
    lm.m.set_transform(tr);
    lm.visible=true;
    lm.need_apply=true;
    m_need_apply=true;

    return mesh_idx;
}

mesh &location::modify_mesh(int idx,bool need_apply)
{
    location_mesh &m=m_meshes.get(idx);
    if(need_apply)
        m_need_apply=m.need_apply=true;
    return m.m;
}

mesh &location::modify_mesh(const char *tag,int idx,bool need_apply)
{
    return modify_mesh(m_meshes.get_idx(tag,idx),need_apply);
}

void location::update(int dt)
{
    if(m_need_apply)
    {
        for(int i=0;i<m_meshes.get_count();++i)
        {
            location_mesh &lm=m_meshes.get(i);

            if(!lm.need_apply)
                continue;

            for(int i=0;i<lm.m.get_groups_count();++i)
            {
                for(size_t j=0;j<m_material_params.size();++j)
                {
                    const int idx=lm.m.get_material(i).get_param_idx(m_material_params[j].first.c_str());
                    if(idx<0)
                        continue;

                    lm.m.modify_material(i).set_param(idx,m_material_params[j].second);
                }
            }

            lm.need_apply=false;
        }

        m_need_apply=false;
    }

    for(int i=0;i<m_meshes.get_count();++i)
        m_meshes.get(i).m.update(dt);
}

void location::draw(const char *pass,const tags &t) const
{
    if(t.get_count()<=1)
    {
        for(int i=0;i<m_meshes.get_count(t.get(0));++i)
        {
            const location_mesh &lm=m_meshes.get(i);
            if(!lm.visible)
                continue;

            lm.m.draw(pass);
        }

        return;
    }

    m_draw_cache.clear();
    m_draw_cache.resize(m_meshes.get_count(),false);
    for(int i=0;i<t.get_count();++i)
    {
        const char *tag=t.get(i);
        for(int j=0;j<m_meshes.get_count(tag);++j)
        {
            const int mesh_idx=m_meshes.get_idx(tag,j);
            if(m_draw_cache[mesh_idx])
                continue;

            const location_mesh &lm=m_meshes.get(mesh_idx);
            if(!lm.visible)
                continue;

            lm.m.draw(pass);
            m_draw_cache[mesh_idx]=true;
        }
    }
}

const char *location::get_material_param_name(int idx) const
{
    if(idx<0 || idx>=get_material_params_count())
        return 0;

    return m_material_params[idx].first.c_str();
}

const material::param_proxy &location::get_material_param(int idx) const
{
    if(idx<0 || idx>=get_material_params_count())
        return nya_memory::invalid_object<material::param_proxy>();

    return m_material_params[idx].second;
}

void location::set_material_param(int idx,const material::param &p)
{
    if(idx<0 || idx>=get_material_params_count())
        return;

    m_material_params[idx].second.set(p);
}

}
