//nya-engine (C) nyan.developer@gmail.com released under the MIT license (see LICENSE)

#pragma once

#include "render/vbo.h"
#include "render/texture.h"
#include "resources/resources.h"
#include "resources/shared_resources.h"
#include "math/matrix.h"

class tsb_anim;

class tmb_model
{
public:
    bool load(nya_resources::resource_data *data);
    void release();

    void draw(bool use_materials,bool cull_face,int group=-1) const;

    unsigned int get_bones_count() const { return (unsigned int)m_bones.size(); }
    const nya_math::mat4 &get_bone(int idx) const { return m_bones[idx]; }

    unsigned int get_groups_count() const { return (unsigned int)m_group_names.size(); }
    const char *get_group_name(unsigned int idx) const
    { 
        if(idx>=m_group_names.size())
            return 0;

        return m_group_names[idx].c_str();
    }

private:
    nya_render::vbo m_vbo;

    struct material
    {
        unsigned int group;
        
        unsigned int vert_offset;
        unsigned int vert_count;
        
        bool cull_face;

        unsigned int tex_idx;
    };

    std::vector<nya_render::texture> m_textures;
    std::vector<material> m_materials;

private:
    std::vector<nya_math::mat4> m_bones;

private:
    std::vector<std::string> m_group_names;

public:
    struct locator
    {      
        float pos[3];
        float ang[3];
        float scale[3];
        
        float color[3];

        //std::string name; //ToDo ?
    };

    const locator *get_locator(int idx) const
    {
        if(idx<0 || idx>=m_locators.size())
            return 0;

        return &m_locators[idx];
    }

private:
    std::vector<locator> m_locators;
};

typedef nya_resources::shared_resources<tmb_model,8> shared_models;
typedef shared_models::shared_resource_ref model_ref;

class shared_models_manager: public shared_models
{
    bool fill_resource(const char *name,tmb_model &res);
    bool release_resource(tmb_model &res);
};

shared_models_manager &get_shared_models();
