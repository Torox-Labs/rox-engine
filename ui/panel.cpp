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


#include "panel.h"

namespace rox_ui
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
