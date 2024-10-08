//nya-engine (C) nyan.developer@gmail.com released under the MIT license (see LICENSE)

#pragma once

#include "widgets.h"

typedef nya_ui::uint uint;

class ui: public nya_ui::layer
{
public:
    void init();
    void draw() override;
    void resize(uint width,uint height) override
    {
        widget_renderer::get().resize(width,height);
        nya_ui::layer::resize(width,height);
    }

    void release() { widget_renderer::get().release(); }

public:
    ui(): m_under_top(false),m_under_bottom(false) {}

private:
    virtual void process_events(const nya_ui::event &e) override;

private:
    bool is_props_visible();
    void update_props_panel();
    void modal(bool enabled,int x,int y);

private:
    nya_ui::widget_base_proxy<slider> m_opac_slider;

    bool m_under_top;
    bool m_under_bottom;

    nya_ui::widget_base_proxy<panel> m_customize_pnl;

    enum custom_mode
    {
        none,
        cos_up,
        cos_dn
    };

    custom_mode m_custom_mode;
    std::string m_customise_group;
    std::vector<std::string> m_customize_groups;

private:
    nya_ui::widget_base_proxy<list> m_customise_lst;
    nya_ui::widget_base_proxy<list> m_anim_lst;
    nya_ui::widget_base_proxy<list> m_scenery_lst;
};
