//nya-engine (C) nyan.developer@gmail.com released under the MIT license (see LICENSE)

#include "slider.h"

namespace nya_ui
{

void slider::update_rects()
{
    rect r=get_rect();
    if(!r.w || !r.h)
        return;

    m_slider_rect=r;

    if(m_vertical)
    {
        m_slider_rect.h=m_slider_size;
        m_slider_rect.y=r.y+int((r.h-m_slider_size)*m_value);
    }
    else
    {
        m_slider_rect.w=m_slider_size;
        m_slider_rect.x=r.x+int((r.w-m_slider_size)*m_value);
    }
}

bool slider::on_mouse_move(uint x,uint y)
{
    widget::on_mouse_move(x,y);
    float last_value=m_value;

    if(m_vertical)
    {
        if(m_mouse_pressed)
        {
            rect r=get_rect();
            if(r.h<1)
                return false;

            const float new_value=float(clamp((y-r.y),0,r.h));
            m_value=new_value/r.h;
        }

        m_mouse_last=y;
    }
    else
    {
        if(m_mouse_pressed)
        {
            rect r=get_rect();
            if(r.w<1)
                return false;

            const float new_value=float(clamp((x-r.x),0,r.w));
            m_value=new_value/r.w;
        }

        m_mouse_last=x;
    }

    if(m_value!=last_value)
    {
        send_to_parent("value_changed");
        update_rects();
    }

    return is_mouse_over();
}

bool slider::on_mouse_button(mouse_button button,bool pressed)
{
    m_mouse_pressed=pressed;

    if(m_vertical)
        on_mouse_move(0,m_mouse_last);
    else
        on_mouse_move(m_mouse_last,0);

    return true;
}

bool slider::on_mouse_scroll(uint x,uint y)
{
    /*
    rect r=get_rect();
    if(r.h<1)
        return false;

    m_value=clamp(m_value+float(y)/r.h,0,1.0f);
    update_rects();
*/
    return true;
}

}
