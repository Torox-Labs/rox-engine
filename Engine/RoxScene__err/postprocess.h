//nya-engine (C) nyan.developer@gmail.com released under the MIT license (see LICENSE)

#pragma once

#include "scene/material.h"
#include "render/screen_quad.h"
#include "render/fbo.h"
#include "render/render.h"
#include "tags.h"

namespace nya_scene
{

struct shared_postprocess
{
    struct line
    {
        std::string type,name;
        std::vector<std::pair<std::string,std::string> > values;

        const char *get_value(const char *name) const;
    };

    std::vector<line> lines;

    bool release() { lines.clear(); return true; }
};

class postprocess: public scene_shared<shared_postprocess>
{
private:
    virtual void draw_scene(const char *pass,const tags &t) {}

public:
    bool load(const char *name);
    void unload();

public:
    bool build(const char *text);

public:
    void resize(unsigned int width,unsigned int height);
    void draw(int dt);

public:
    void set_condition(const char *condition,bool value);
    bool get_condition(const char *condition) const;
    void clear_conditions();

    void set_variable(const char *name,float value);
    float get_variable(const char *name) const;
    
    void set_texture(const char *name,const texture_proxy &tex);
    const texture_proxy &get_texture(const char *name) const;

    void set_shader_param(const char *name,const nya_math::vec4 &value);
    const nya_math::vec4 &get_shader_param(const char *name) const;

public:
    unsigned int get_width() const { return m_width; };
    unsigned int get_height() const { return m_height; };

public:
    postprocess(): m_width(0),m_height(0),m_auto_resize(true) {}
    ~postprocess() { unload(); }

public:
    static bool load_text(shared_postprocess &res,resource_data &data,const char* name);

private:
    void update();
    void update_shader_param(int idx);
    void clear_ops();
    void unload_internal();

private:
    unsigned int m_width,m_height;
    bool m_auto_resize;
    nya_memory::shared_ptr<nya_render::screen_quad> m_quad;

    struct tex_holder
    {
        bool user_set; texture_proxy tex;
        tex_holder() {} tex_holder(bool u,const texture_proxy &t): user_set(u),tex(t) {}
    };

    std::vector<std::pair<std::string,bool> > m_conditions;
    std::vector<std::pair<std::string,float> > m_variables;
    std::vector<std::pair<std::string,tex_holder> > m_textures;
    std::vector<std::pair<std::string,nya_math::vec4> > m_shader_params;

    enum op_types
    {
        type_set_target,
        type_set_texture,
        type_set_shader,
        type_set_material,
        type_clear,
        type_draw_scene,
        type_draw_quad
    };

    struct op
    {
        op_types type;
        size_t idx;
    };
    std::vector<op> m_op;

    struct op_draw_scene
    {
        std::string pass;
        tags t;
    };
    std::vector<op_draw_scene> m_op_draw_scene;

    struct op_clear
    {
        bool color,depth;
        nya_math::vec4 color_value;
        float depth_value;
    };
    std::vector<op_clear> m_op_clear;

    struct op_set_shader
    {
        shader sh;
        std::vector<int> params_map;
        std::vector<std::pair<int,nya_math::vec4> > params;
    };
    std::vector<op_set_shader> m_op_set_shader;

    struct op_set_material
    {
        material mat;
        std::vector<int> params_map;
        std::vector<std::pair<int,nya_math::vec4> > params;
    };
    std::vector<op_set_material> m_op_set_material;

    struct op_set_texture
    {
        size_t tex_idx;
        int layer;
    };
    std::vector<op_set_texture> m_op_set_texture;

    struct op_target
    {
        nya_scene::proxy<nya_render::fbo> fbo;
        nya_render::rect rect;
        int color_idx, depth_idx;
        int samples;

        op_target(): color_idx(-1),depth_idx(-1),samples(1) {}
    };
    std::vector<op_target> m_targets;
};

}
