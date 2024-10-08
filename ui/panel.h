//nya-engine (C) nyan.developer@gmail.com released under the MIT license (see LICENSE)

#pragma once

#include "ui/ui.h"

namespace nya_ui
{

class panel: public widget, public layout
{
public:
    virtual void set_pos(int x,int y) override;
    virtual void set_size(uint width,uint height) override;

protected:
    virtual void draw(layout &l) override { layout::draw_widgets(l); }
    virtual void process(uint dt,layout &parent) override
    {
        layout::process_widgets(dt,*this);
        widget::process(dt,parent);
    }

    virtual void process_events(const event &e) override;
    virtual void parent_resized(uint width,uint height) override;
    virtual void parent_moved(int x,int y) override;
    virtual void calc_pos_markers() override;

public:
    virtual void add_widget_proxy(const widget_proxy &w) override
    {
        if(w.operator->()==this)
            return;

        layout::add_widget_proxy(w);
    }

protected:
    virtual void on_mouse_left() override { layout::mouse_left(); widget::on_mouse_left(); }
    virtual bool on_mouse_move(uint x,uint y) override
    {
        layout::mouse_move(x,y);
        widget::on_mouse_move(x,y);
        return is_mouse_over();
    }

    virtual bool on_mouse_scroll(uint dx,uint dy) override
    {
        layout::mouse_scroll(dx,dy);
        return true;
    }

    virtual bool on_mouse_button(enum mouse_button button,bool pressed) override
    {
        layout::mouse_button(button,pressed);
        return true;
    }

public:
    virtual void send_event(const event &e) override { send_to_parent(e); }

protected:
    void update_layout_rect();
};

}
