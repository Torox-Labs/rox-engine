//nya-engine (C) nyan.developer@gmail.com released under the MIT license (see LICENSE)

#include "widgets.h"

#include "render/render.h"
#include "memory/tmp_buffer.h"

void widget_renderer::init()
{
    m_text_shader.add_program(nya_render::shader::vertex,
                              "varying vec2 tc;"

                              "void main()"
                              "{"
                              "    tc=gl_MultiTexCoord0.xy;"
                              "    gl_Position=vec4(gl_Vertex.xyz,1.0);"
                              "}");
    
    m_text_shader.add_program(nya_render::shader::pixel,
                              "uniform sampler2D alpha_map;"
                              "varying vec2 tc;"
                              "uniform vec4 color;"
                              
                              "void main(void)"
                              "{"
                              "    float alpha=texture2D(alpha_map,tc.xy).r;"
                              "    gl_FragColor=vec4(color.rgb,alpha);"
                              "}");

    m_text_shader_color=m_text_shader.find_uniform("color");
    m_text_shader.set_uniform(m_text_shader_color,1.0f,1.0f,1.0f,1.0f);

    m_ui_shader.add_program(nya_render::shader::vertex,
                            //"varying vec4 tc;"

                            "void main()"
                            "{"
                            //"    tc=gl_MultiTexCoord0;"
                            "    gl_Position=vec4(gl_Vertex.xyz,1.0);"
                            "}");
    
    m_ui_shader.add_program(nya_render::shader::pixel,
                            //"uniform sampler2D base_map;"
                            //"varying vec4 tc;"
                            "uniform vec4 color;"
                            
                            "void main(void)"
                            "{"
                            //"    vec4 base=texture2D(base_map,gl_TexCoord[0].xy);"
                            //"    gl_FragColor=gl_TexCoord[1]*base;"
                            "    gl_FragColor=color;"
                            "}");

    m_ui_shader_color=m_ui_shader.find_uniform("color");

    m_font_tex=nya_resources::get_shared_textures().access("font.tga");
    //m_ui_tex=nya_resources::get_shared_textures().access("ui.tga");

    m_font_vbo.set_vertices(0,2);
    m_font_vbo.set_tc(0,sizeof(float)*2,2);
    m_rect_vbo.set_vertices(0,2);
}

void widget_renderer::release()
{
    m_font_tex.free();
    m_ui_shader.release();
    m_text_shader.release();
    m_font_vbo.release();
    m_rect_vbo.release();
}

void widget_renderer::draw_text(uint x,uint y,const char *text
                      ,font_align aligh_hor,font_align aligh_vert)
{
    if(!text)
        return;

    std::string text_str(text);
    if(text_str.empty())
        return;

    if(!m_font_tex.is_valid())
        return;

    m_text_shader.bind();
    m_font_tex->bind(0);

    const int str_len=(int)text_str.size();
    
    //font_params
    const uint font_width=256;
    const uint font_height=128;
    const uint char_size=16;
    const uint char_offs=1;
    
    const uint char_actual_width=8;//bad magic: should
    uint char_widths[128-32]; //be font-defined array
    for(int i=0;i<128-32;++i)
    {
        char c=i+32;
        if(c=='i'||c=='j' || c=='l')
            char_widths[i]=5;
        else if(c=='m')
            char_widths[i]=10;
        else
            char_widths[i]=char_actual_width;
    }
    
    //
    const float font_scale=1.0f;
    
    //precomputed from font_params
    const int chars_per_row=font_width/char_size;
    const float tc_w=float(char_size)/font_width;
    const float tc_h=float(char_size)/font_height;
    
    const float offs_w=float(char_offs)/font_width;
    const float offs_h=float(char_offs)/font_height;
    
    float chs=2.0f*font_scale*(char_size-char_offs);
    
    //=====
    
    const float w=chs/m_width;
    const float h=chs/m_height;
    
    if(aligh_hor==center)
        x-=int(0.25f*(chs*char_actual_width/char_size*str_len));
    else if(aligh_hor==right)
        x-=int(0.5f*(chs*char_actual_width/char_size*str_len));
    
    if(aligh_vert==center)
        y-=int(0.25f*chs);
    else if(aligh_vert==top)
        y-=int(0.5f*chs);
    
    float px=-1.0f+2.0f*x/m_width;
    float py=-1.0f+2.0f*y/m_height;

    const uint elem_per_char=6;
    const uint vert_count=str_len*elem_per_char;
    if(m_font_vertices.size()<vert_count)
        m_font_vertices.resize(vert_count);

    float dpos=0;
    for(int i=0;i<str_len;++i)
    {
        const char c=text_str[i];
        
        if(c<32 || c>127)
            continue;
        
        font_vertex *v=&m_font_vertices[elem_per_char*i];
        
        const uint char_width=char_widths[c-32];
        
        v[2].x=v[5].x=px+dpos;
        v[1].x=v[0].x=v[2].x+w*char_width/char_size;
        v[0].y=v[5].y=py;
        v[2].y=v[1].y=py+h;
        
        const uint letter_pos=c-32;
        const uint letter_x=(letter_pos)%chars_per_row;
        const uint letter_y=(letter_pos)/chars_per_row;
        
        const float tcx=tc_w*letter_x-offs_w;
        const float tcy=tc_h*letter_y;
        float tcw=tc_w-offs_w;
        float tch=tc_h-offs_h;
        
        const float tc_fix=0.5f*(tc_w-float(char_width)/font_width);
        
        v[5].s=v[2].s=tcx+tc_fix;
        v[1].s=v[0].s=tcx+tcw-tc_fix;
        v[5].t=v[0].t=tcy+tch;
        v[2].t=v[1].t=tcy;
        
        v[3]=v[0];
        v[4]=v[2];
        
        dpos+=w*char_width/char_size;
    }

    m_font_vbo.set_vertex_data(m_font_vertices.data(),sizeof(font_vertex),(int)m_font_vertices.size(),nya_render::vbo::dynamic_draw);
    m_font_vbo.bind();
    m_font_vbo.draw(vert_count);
    m_font_vbo.unbind();

    m_font_tex->unbind(0);
    m_text_shader.unbind();
}

void widget_renderer::draw_rect(nya_ui::rect &r,rect_style &s)
{
    if(!s.border&&!s.solid)
        return;
    
    float w = 2.0f*r.w/m_width;
    float h = 2.0f*r.h/m_height;
    
    float px=-1.0f+2.0f*r.x/m_width;
    float py=-1.0f+2.0f*r.y/m_height;
    
    float pos[4][2];
    pos[0][0]=pos[2][0]=px;
    pos[0][1]=pos[1][1]=py;
    pos[2][1]=pos[3][1]=h+py;
    pos[1][0]=pos[3][0]=w+px;
    
    m_rect_vbo.set_vertex_data(pos,sizeof(float)*2,4,nya_render::vbo::dynamic_draw);
    static bool initialised=false;
    if(!initialised)
    {
        initialised=true;
        m_rect_vbo.set_vertices(0,2);
        unsigned short indices[5]={1,0,2,3,1};
        m_rect_vbo.set_index_data(indices,nya_render::vbo::index2b,5);
    }

    m_rect_vbo.bind_verts();
    m_ui_shader.bind();

    if(s.solid)
    {
        m_ui_shader.set_uniform(m_ui_shader_color,s.solid_color.r,s.solid_color.g,
                                                  s.solid_color.b,s.solid_color.a);
        
        m_rect_vbo.set_element_type(nya_render::vbo::triangle_strip);
        
        m_rect_vbo.draw();
    }

    if(s.border)
    {
        m_rect_vbo.bind_indices();
        m_ui_shader.set_uniform(m_ui_shader_color,s.border_color.r,s.border_color.g,
                                s.border_color.b,s.border_color.a);

        m_rect_vbo.set_element_type(nya_render::vbo::line_strip);
        m_rect_vbo.draw();
    }

    m_rect_vbo.unbind();
    m_ui_shader.unbind();
}

void widget_renderer::set_scissor(nya_ui::rect &r)
{
    nya_render::scissor::enable(r.x,r.y,r.w,r.h);
}

void widget_renderer::remove_scissor()
{
    nya_render::scissor::disable();
}

void button::draw(nya_ui::layout &l)
{
    nya_ui::rect r=get_rect();
    if(!r.w || !r.h)
        return;
    
    widget_renderer::rect_style rs;
    
    if(m_mouse_over && !m_mouse_pressed)
        rs.border_color.set(0.7f,0.6f,1.0f,1.0f);
    else
        rs.border_color.set(0.4f,0.3f,1.0f,1.0f);
    
    rs.border=true;
    
    rs.solid_color=rs.border_color;
    rs.solid_color.g+=0.05f;
    rs.solid_color.b-=0.1f;
    rs.solid_color.a=0.8f;
    rs.solid=true;
    
    widget_renderer::get().draw_rect(r,rs);
    
    if(!m_text.empty())
        widget_renderer::get().draw_text(r.x+r.w/2,r.y+r.h/2,m_text.c_str(),widget_renderer::center,widget_renderer::center);
}

void label::draw(nya_ui::layout &l)
{
    nya_ui::rect r=get_rect();
    if(!r.w || !r.h)
        return;
    
    if(m_text.empty())
        return;
    
    widget_renderer::get().draw_text(r.x+r.w/2,r.y+r.h/2,m_text.c_str(),widget_renderer::center,widget_renderer::center);
}

void panel::draw(nya_ui::layout &l)
{
    nya_ui::rect r=get_rect();
    if(!r.w || !r.h)
        return;

    widget_renderer::get().draw_rect(r,m_style.panel);

    nya_ui::panel::draw(l);
}

void slider::draw(nya_ui::layout &l)
{
    nya_ui::rect r=get_rect();
    if(!r.w || !r.h)
        return;

    widget_renderer::get().draw_rect(r,m_style.area);
    widget_renderer::get().draw_rect(m_slider_rect,m_style.slider);
}

void list::draw(nya_ui::layout &l)
{
    nya_ui::rect r=get_rect();
    if(!r.w || !r.h)
        return;
    
    widget_renderer::get().draw_rect(r,m_style.list);
    widget_renderer::get().draw_rect(m_scroll_area_rect,m_style.scroll_area);
    
    if(m_button_up_rect.check_point(m_mouse_pos))
        widget_renderer::get().draw_rect(m_button_up_rect,m_style.button_up_hl);
    else
        widget_renderer::get().draw_rect(m_button_up_rect,m_style.button_up);
    
    if(m_button_down_rect.check_point(m_mouse_pos))
        widget_renderer::get().draw_rect(m_button_down_rect,m_style.button_dn_hl);
    else
        widget_renderer::get().draw_rect(m_button_down_rect,m_style.button_dn);
    
    nya_ui::rect er;
    er.h=m_entry_height;
    er.w=r.w-m_scroll_area_rect.w;
    er.x=r.x;
    
    if(er.h*m_elements.size()>r.h)
    {
        if(m_scroll_rect.check_point(m_mouse_pos))
            widget_renderer::get().draw_rect(m_scroll_rect,m_style.scroll_hl);
        else
            widget_renderer::get().draw_rect(m_scroll_rect,m_style.scroll);
    }
    
    widget_renderer::get().set_scissor(r);
    
    long y=r.y+r.h-er.h;
    if(m_scroll_max)
        y+=(er.h*m_elements.size()-r.h)*m_scroll/m_scroll_max;
    
    for(int i=0;i<(int)m_elements.size();++i,y-=m_entry_height)
    {
        if(y+er.h<r.y||y>(int)(r.y+r.h))
            continue;
        
        er.y=(unsigned int)y;
        
        if(i==m_mover && !m_mouse_pressed)
            widget_renderer::get().draw_rect(er,m_style.entry_hl);
        else if(i==m_selected)
            widget_renderer::get().draw_rect(er,m_style.entry_selected);
        else
            widget_renderer::get().draw_rect(er,m_style.entry);
        
        widget_renderer::get().draw_text(er.x,er.y+er.h/2,m_elements[i].c_str(),widget_renderer::left,widget_renderer::center);
    }
    
    widget_renderer::get().remove_scissor();
}
