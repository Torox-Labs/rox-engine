//nya-engine (C) nyan.developer@gmail.com released under the MIT license (see LICENSE)

#include "ui/ui.h"

#include <string>
#include <string.h>

namespace { nya_log::log_base *ui_log=0; }

namespace nya_ui
{

uint clamp(int v,uint from,uint to)
{
    if(v>(int)to) v=to;
    if(v<(int)from) v=from;
    return v;
}

float clamp(float v,float from,float to)
{
    if(v>(int)to) v=to;
    if(v<(int)from) v=from;
    return v;
}


const rect &widget::get_rect()
{
    if(m_cached_rect)
        return m_rect;

    int x,y,w,h;
    if(m_align_left)
    {
        x=m_pos_left+m_parent_rect.x;
        if(m_align_right)
            w=m_parent_rect.w-m_pos_right-m_pos_left;
        else
            w=m_width;
    }
    else if(m_align_right)
    {
        x=m_parent_rect.x+m_parent_rect.w-m_width-m_pos_right;
        w=m_width;
    }
    else
    {
        x=m_parent_rect.x+m_parent_rect.w/2-m_center_x-m_width/2;
        w=m_width;
    }
    
    if(m_align_bottom)
    {
        y=m_pos_bottom+m_parent_rect.y;
        if(m_align_top)
            h=m_parent_rect.h-m_pos_top-m_pos_bottom;
        else
            h=m_height;
    }
    else if(m_align_top)
    {
        y=m_parent_rect.y+m_parent_rect.h-m_height-m_pos_top;
        h=m_height;
    }
    else
    {
        y=m_parent_rect.y+m_parent_rect.h/2-m_center_y-m_height/2;
        h=m_height;
    }
    
    if(m_keep_aspect==from_width)
    {
        h=w*m_height/m_width;
    }
    else if(m_keep_aspect==from_height)
    {
        w=h*m_width/m_height;
    }

    /*
    if(x<m_parent_rect.x)
        x=m_parent_rect.x;
    if(y<m_parent_rect.y)
        y=m_parent_rect.y;

    const int cw=m_parent_rect.x-x+m_parent_rect.w;
    if(w>cw)
        w=cw;
    
    const int ch=m_parent_rect.y-y+m_parent_rect.h;
    if(h>ch)
        h=ch;
    */

    m_rect=rect();

    m_rect.x=x;
    m_rect.y=y;
    if(w>0) m_rect.w=w;
    if(h>0) m_rect.h=h;
    
    m_cached_rect=true;
    
    return m_rect;
}

void widget::process(uint dt,layout &parent)
{
    if(m_events_to_parent.empty())
        return;

    events_deque events=m_events_to_parent;
    m_events_to_parent.clear();

    for(events_deque::const_iterator it=events.begin();it!=events.end();++it)
        parent.send_event(*it);//,printf("e %s %s\n",it->sender.c_str(),it->type.c_str());
}


void layer::draw()
{
    draw_widgets(*this);
}

void layer::resize(uint width, uint height)
{
    m_rect.w=width;
    m_rect.h=height;

    layout::resize(m_rect.w,m_rect.h);
}

void layer::process(uint dt)
{
    process_widgets(dt,*this);
    if(m_events.empty())
        return;

    events_deque events=m_events;
    m_events.clear();

    for(events_deque::iterator it=events.begin();it!=events.end();++it)
        process_events(*it);
}

void layout::process_events(const event &e)
{
    for(widgets_list::iterator it=m_widgets.begin();
        it!=m_widgets.end();++it)
    {
        if(it->is_valid())
            (*it)->process_events(e);
    }
}

void layout::add_widget_proxy(const widget_proxy &w)
{
    if(!w.is_valid())
        return;

    m_widgets.push_back(w);

    m_widgets.back()->parent_moved(m_rect.x,m_rect.y);
    m_widgets.back()->parent_resized(m_rect.w,m_rect.h);
    m_widgets.back()->calc_pos_markers();
}

widget_proxy layout::get_widget(const char *name)
{
    if(!name)
        return widget_proxy();

    for(widgets_list::iterator it=m_widgets.begin();
        it!=m_widgets.end();++it)
    {
        widget_proxy &w=*it;
        const char *id=w.get_id();
        if(!id)
            continue;

        if(strcmp(name,id)==0)
            return w;
    }

    return widget_proxy();
}

void layout::process_widgets(uint dt,layout &l)
{
    for(widgets_list::iterator it=m_widgets.begin();
        it!=m_widgets.end();++it)
    {
        widget_proxy &w=*it;
        if(w.is_valid())
            w->process(dt,l);
    }
}

void layout::draw_widgets(layout &l)
{
    if(!m_rect.w || !m_rect.h)
        return;

    for(widgets_list::iterator it=m_widgets.begin();
        it!=m_widgets.end();++it)
    {
        widget_proxy &w=*it;
        if(w.is_visible()) //also means valid
            w->draw(l);
    }
}

void layout::resize(uint width,uint height)
{
    m_rect.w=width;
    m_rect.h=height;

    for(widgets_list::iterator it=m_widgets.begin();
        it!=m_widgets.end();++it)
    {
        widget_proxy &w=*it;
        if(w.is_valid())
            w->parent_resized(m_rect.w,m_rect.h);
    }
}

void layout::move(int x,int y)
{
    m_rect.x=x;
    m_rect.y=y;

    for(widgets_list::iterator it=m_widgets.begin();
        it!=m_widgets.end();++it)
    {
        widget_proxy &w=*it;
        if(w.is_valid())
            w->parent_moved(m_rect.x,m_rect.y);
    }
}

void layout::remove_widget(const char *name)
{
    if(!name)
        return;

    for(widgets_list::iterator it=m_widgets.begin();
        it!=m_widgets.end();)
    {
        const char *n=it->get_id();
        if(n && strcmp(name,n)==0)
        {
            widgets_list::iterator itn=it;
            ++itn;
            m_widgets.erase(it);
            it=itn;
        }
        else
            ++it;
    }
}

bool layout::mouse_button(enum mouse_button button,bool pressed)
{
    bool processed=false;

    for(widgets_list::reverse_iterator it=m_widgets.rbegin();
          it!=m_widgets.rend();++it)
    {
        widget_proxy &w=*it;
        if(!w.is_visible()) //visible also means valid
            continue;

        if(((w->is_mouse_over() && !processed) || !pressed)
            && w->m_mouse_pressed!=pressed)
        {
            const bool result=w->on_mouse_button(button,pressed);
            w->m_mouse_pressed=pressed;
            if(pressed && result)
                processed=true;
        }
    }

    return processed;
}

bool layout::mouse_move(uint x,uint y)
{
    m_mouse_pos.x=x;
    m_mouse_pos.y=y;

    bool processed=false;

    for(widgets_list::reverse_iterator it=m_widgets.rbegin();
          it!=m_widgets.rend();++it)
    {
        widget_proxy &w=*it;
        if(!w.is_visible()) //also means valid
            continue;

        if(!processed && w->on_mouse_move(x,y))
            processed=true;
    }

    return processed;
}

void layout::mouse_left()
{
    for(widgets_list::iterator it=m_widgets.begin();
        it!=m_widgets.end();++it)
    {
        widget_proxy &w=*it;
        if(w.is_mouse_over()) //also means valid
        {
            w->on_mouse_left();
            w->m_mouse_over=false;
        }
    }
}

bool layout::mouse_scroll(uint dx,uint dy)
{
    bool processed=false;
    for(widgets_list::iterator it=m_widgets.begin();
        it!=m_widgets.end();++it)
    {
        widget_proxy &w=*it;
        if(!w.is_visible()) //also means valid
            continue;

        if(w->is_mouse_over())
        {
            if(w->on_mouse_scroll(dx,dy))
                processed=true;
        }
    }

    return processed;
}


void layer::send_event(const event &e)
{
    m_events.push_back(e);

    const uint msg_limit=1024;

    if(m_events.size()>msg_limit)
        m_events.pop_front();
}


void set_log(nya_log::log_base *l) { ui_log=l; }

nya_log::log_base &log()
{
    if(!ui_log)
        return nya_log::log();

    return *ui_log;
}

}
