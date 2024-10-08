//nya-engine (C) nyan.developer@gmail.com released under the MIT license (see LICENSE)

#pragma once

#include "ui/ui.h"

namespace nya_ui
{

class label: public widget
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

protected:
    std::string m_text;
};

}
