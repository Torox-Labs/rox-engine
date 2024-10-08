//nya-engine (C) nyan.developer@gmail.com released under the MIT license (see LICENSE)

#pragma once

#include "ui/ui.h"
#include "ui/button.h"
#include "ui/list.h"
#include "ui/panel.h"
#include "ui/slider.h"
#include "ui/label.h"

#include "shared_textures.h"

#include "render/shader.h"
#include "render/vbo.h"

typedef unsigned int uint ;

class widget_renderer
{
public:
    void init();
    void release();
    void resize(uint width,uint height) { m_width=width; m_height=height; }

public:
    static widget_renderer &get()
    {
        static widget_renderer renderer;
        return renderer;
    }

public:
    struct color
    {
        float r;
        float g;
        float b;
        float a;
        
        void set(float r,float g,float b,float a=1.0f)
        {
            this->r=r;
            this->g=g;
            this->b=b;
            this->a=a;
        }
        
        color(): r(0),g(0),b(0),a(1.0f) {}
        color(float r,float g,float b,float a=1.0f):
        r(r), g(g), b(b), a(a) {}
    };
    
    struct font
    {
        //std::string name;
        uint font_width;
        uint font_height;
        uint char_size;
        uint char_offs;
    };
    
    enum font_align
    {
        left,
        right,
        top,
        bottom,
        center
    };
    
    virtual void draw_text(uint x,uint y,const char *text
                           ,font_align aligh_hor=left,font_align aligh_vert=bottom);
    virtual void draw_text(uint x,uint y,const char *text,font &f
                           ,font_align aligh_hor=left,font_align aligh_vert=bottom) {}
    
    struct rect_style
    {
        bool border;
        bool solid;
        
        color border_color;
        color solid_color;
        
        //std::string tex_name;
        //rect tc_rect;
        
        rect_style(): border(false), solid(false) {}
    };

    virtual void draw_rect(nya_ui::rect &r,rect_style &s);
    virtual void set_scissor(nya_ui::rect &r);
    virtual void remove_scissor();

private:
    nya_resources::texture_ref m_font_tex;
    nya_render::shader m_ui_shader,m_text_shader;
    int m_ui_shader_color,m_text_shader_color;
    nya_render::vbo m_rect_vbo;
    nya_render::vbo m_font_vbo;

    struct font_vertex { float x,y,s,t; };
    std::vector<font_vertex> m_font_vertices;

private:
    uint m_width;
    uint m_height;
};

class button: public nya_ui::button { void draw(nya_ui::layout &layer) override; };
class label: public nya_ui::label { void draw(nya_ui::layout &layer) override; };

class panel: public nya_ui::panel
{
public:
    struct style
    {
        widget_renderer::rect_style panel;

        style()
        {
            panel.border_color.set(0.4f,0.3f,1.0f,1.0f);
            panel.border=true;
        }
    };

    void set_style(style &s) { m_style=s; }

private:
    void draw(nya_ui::layout &l) override;

private:
    style m_style;
};

class slider: public nya_ui::slider
{
public:
    struct style
    {
        widget_renderer::rect_style area;
        widget_renderer::rect_style slider;
        widget_renderer::rect_style slider_hl;

        style()
        {
            area.border=true;
            area.solid=true;
            area.border_color.set(0.4f,0.3f,1.0f,1.0f);
            area.solid_color=area.border_color;
            area.solid_color.a=0.2f;
            slider=area;
            slider.solid_color.a=0.5f;
            slider_hl=slider;
            slider_hl.solid_color.set(0.7f,0.6f,1.0f,0.5f);
        }
    };
    
    void set_style(style &s) { m_style=s; }

private:
    void draw(nya_ui::layout &l) override;

private:
    style m_style;
};

class list: public nya_ui::list
{
public:
    struct style
    {
        widget_renderer::rect_style list;
        widget_renderer::rect_style entry;
        widget_renderer::rect_style entry_hl;
        widget_renderer::rect_style entry_selected;
        widget_renderer::rect_style scroll;
        widget_renderer::rect_style scroll_hl;
        widget_renderer::rect_style scroll_area;
        widget_renderer::rect_style button_up;
        widget_renderer::rect_style button_dn;
        widget_renderer::rect_style button_up_hl;
        widget_renderer::rect_style button_dn_hl;
        
        style()
        {
            list.border=true;
            list.border_color.set(0.4f,0.3f,1.0f,1.0f);
            
            entry=list;
            
            list.solid=true;
            list.solid_color=list.border_color;
            list.solid_color.a=0.4f;
            
            scroll_area=list;
            scroll_area.solid_color.a=0.5f;
            
            scroll=list;
            scroll.solid_color.a=0.8f;
            
            button_up=button_dn=scroll;
            
            entry_selected=list;
            
            entry_selected.solid_color.a=0.6f;
            entry_hl=entry;
            entry_hl.solid=true;
            entry_hl.solid_color.set(0.7f,0.6f,1.0f,0.8f);
            scroll_hl=button_dn_hl=button_up_hl=entry_hl;
        }
    };
    
    void set_style(style &s) { m_style=s; }

private:
    void draw(nya_ui::layout &layer) override;
    
private:
    style m_style;
};
