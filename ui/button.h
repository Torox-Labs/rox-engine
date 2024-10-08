//nya-engine (C) nyan.developer@gmail.com released under the MIT license (see LICENSE)

#pragma once

#include "ui/ui.h"

namespace nya_ui
{

class button: public widget
{
public:
    virtual void set_text(const char *text)
    {
        if(!text)
            return;

        m_text.assign(text);
    }

protected:
    virtual void draw(layout &l) override {}

    virtual bool on_mouse_button(mouse_button button,bool pressed) override
    {
        send_to_parent(pressed?"mouse_btn_down":"mouse_btn_up");

        if(!pressed && is_mouse_over())
            send_to_parent("button_pressed");

        return true;
    }

protected:
    std::string m_text;
};

}
