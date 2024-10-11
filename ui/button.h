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
