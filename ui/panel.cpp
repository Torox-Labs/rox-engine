//nya-engine (C) nyan.developer@gmail.com released under the MIT license (see LICENSE)

#include "panel.h"

namespace nya_ui
{

void panel::process_events(const event &e)
{
    layout::process_events(e);
}

void panel::set_pos(int x,int y)
{
    widget::set_pos(x,y);
    update_layout_rect();
}

void panel::set_size(uint width,uint height)
{
    widget::set_size(width,height);
    update_layout_rect();
}

void panel::parent_resized(uint width,uint height)
{
    widget::parent_resized(width,height);
    update_layout_rect();
}

void panel::parent_moved(int x,int y)
{
    widget::parent_moved(x,y);
    update_layout_rect();
}

void panel::calc_pos_markers()
{
    widget::calc_pos_markers();
    update_layout_rect();
}

void panel::update_layout_rect()
{
    rect r=get_rect();
    layout::move(r.x,r.y);
    layout::resize(r.w,r.h);
}

}
