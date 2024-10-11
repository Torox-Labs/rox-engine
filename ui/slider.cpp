// Copyright © 2024 Torox Project
// Portions Copyright © 2013 nyan.developer@gmail.com (nya-engine)
//
// This file was modified by the Torox Project. The namespace has been renamed from nya_ui to rox_iu.
// 
// This file incorporates code from the nya-engine project, which is licensed under the MIT License.
// See the LICENSE-MIT file in the root directory for more information.
//
// This file is also part of the Rox-engine, which is licensed under a dual-license system:
// 1. Free Use License (for non-commercial and commercial use under specific conditions)
// 2. Commercial License (for use on proprietary platforms)
// See the LICENSE file in the root directory for the full Rox-engine license terms.


#include "slider.h"

namespace rox_ui
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
