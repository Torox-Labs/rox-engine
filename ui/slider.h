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


#pragma once

#include "ui/ui.h"

namespace rox_ui
{

class slider: public widget
{
public:
    float get_value() { return m_value; }
    void set_value(float value) { m_value=clamp(value,0.0f,1.0f); update_rects(); }

    virtual void set_slider_size(uint size) //default is 10
    {
        m_slider_size=size;
        update_rects();
    }

    virtual void set_size(uint width,uint height) override
    {
        m_vertical=width<height;
        widget::set_size(width,height);
    }

protected:
    virtual void draw(layout &l) override {}
    virtual void update_rects();

protected:
    virtual bool on_mouse_move(uint x,uint y) override;
    virtual bool on_mouse_button(mouse_button button,bool pressed) override;
    virtual bool on_mouse_scroll(uint x,uint y) override;

protected:
    virtual void parent_moved(int x,int y) override
    {
        widget::parent_moved(x,y);
        update_rects();
    }

    virtual void parent_resized(uint width,uint height) override
    {
        widget::parent_resized(width,height);
        update_rects();
    }

    virtual void calc_pos_markers() override
    {
        widget::calc_pos_markers();
        update_rects();
    }

public:
    slider(): m_value(0),m_mouse_last(0),m_slider_size(10),m_vertical(false) {}

protected:
    float m_value;
    uint m_mouse_last;

protected:
    uint m_slider_size;
    rect m_slider_rect;
    bool m_vertical;
};

}
