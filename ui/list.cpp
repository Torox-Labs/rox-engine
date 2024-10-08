//nya-engine (C) nyan.developer@gmail.com released under the MIT license (see LICENSE)

#include "ui/list.h"
#include "math.h"

namespace nya_ui
{

void list::update_rects()
{
    rect r=get_rect();
    if(!r.w || !r.h)
        return;

    uint last_height=m_scroll_area_rect.h
                    -m_scroll_height;

    rect &scroll_area=m_scroll_area_rect;
    scroll_area.w=m_scroll_width;
    scroll_area.h=r.h-m_button_height*2;
    scroll_area.x=r.x+r.w-m_scroll_width;
    scroll_area.y=r.y+m_button_height;

    if(last_height)
        m_scroll=m_scroll*(scroll_area.h
                    -m_scroll_height)/last_height;
    else
        m_scroll=0;

    rect &button=m_button_down_rect;
    button.w=scroll_area.w;
    button.x=scroll_area.x;
    button.h=m_button_height;
    button.y=r.y;

    m_button_up_rect=m_button_down_rect;
    rect &button_up=m_button_up_rect;
    button_up.y=r.y+r.h-m_button_height;

    rect &scroll=m_scroll_rect;
    scroll.x=scroll_area.x+(m_scroll_area_width
                                -m_scroll_width)/2;
    scroll.y=r.y+r.h-m_button_height
                                -m_scroll_height-m_scroll;
    scroll.w=m_scroll_width;
    scroll.h=m_scroll_height;

    int scroll_max=scroll_area.h-m_scroll_height;
    if(scroll_max<0)
        scroll_max=0;
    m_scroll_max=scroll_max;
}

bool list::on_mouse_move(uint x,uint y)
{
    widget::on_mouse_move(x,y);

    bool mleft=false;
    if(is_mouse_over() && int(x)<m_scroll_area_rect.x)
    {
        rect r=get_rect();

        int scrl=0;
        if(m_scroll_max>0)
            scrl=(int)(m_entry_height*m_elements.size()-r.h)*m_scroll/m_scroll_max;

        int num=0;
        if(m_entry_height>0)
            num=(r.h-(m_mouse_pos.y-r.y)+scrl)/m_entry_height;

        if(num<(int)m_elements.size())
        {
            if(num!=m_mover)
            {
                m_mover=num;
                send_to_parent("mover_element");
            }
        }
        else
            mleft=true;
    }
    else
        mleft=true;

    if(mleft && m_mover>=0)
    {
        m_mover= -1;
        send_to_parent("mleft_elements");
    }

    if(m_scrolling)
    {
        const int new_scroll=m_scroll_max-(m_mouse_pos.y
                -m_scroll_area_rect.y-m_scroll_rect.h/2);
        m_scroll=clamp(new_scroll,0,m_scroll_max);
        update_rects();

        //m_scroll_abs=(m_style.entry_height*m_elements.size()-r.h)*m_scroll/m_scroll_max;

        return true;
    }

    return false;
}

bool list::on_mouse_button(mouse_button button,bool pressed)
{
    m_scrolling=false;

    if(pressed)
    {
        rect r=get_rect();

        if(m_entry_height*m_elements.size()>r.h)
        {
            if(m_scroll_rect.check_point(m_mouse_pos)
               || m_scroll_area_rect.check_point(m_mouse_pos))
            {
                m_scrolling=true;
                on_mouse_move(m_mouse_pos.x,m_mouse_pos.y);
                return true;
            }

            if(m_button_up_rect.check_point(m_mouse_pos))
            {
                const int delta=(int)ceilf(m_scroll_max*0.1f);
                m_scroll=clamp(m_scroll-delta,0,m_scroll_max);
                update_rects();
                return true;
            }

            if(m_button_down_rect.check_point(m_mouse_pos))
            {
                const int delta=(int)ceilf(m_scroll_max*0.1f);
                m_scroll=clamp(m_scroll+delta,0,m_scroll_max);
                update_rects();
                return true;
            }
        }

        if(m_mover>=0 && m_mover<(int)m_elements.size())
        {
            m_selected=m_mover;
            send_to_parent("select_element");
        }

        m_mouse_hold_y=m_mouse_pos.y;
    }

    return true;
}

bool list::on_mouse_scroll(uint x,uint y)
{
    rect r=get_rect();

    if(m_entry_height*m_elements.size()>r.h)
    {
        const int delta=y;//(int)ceilf(m_scroll_max*0.01f)*y;
        m_scroll=clamp(m_scroll-delta,0,m_scroll_max);
        update_rects();

        on_mouse_move(m_mouse_pos.x,m_mouse_pos.y);
    }

    return true;
}

}
