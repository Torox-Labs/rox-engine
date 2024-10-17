// Copyright (C) 2024 Torox Project
// Portions Copyright (C)) 2013 nyan.developer@gmail.com (nya-engine)
//
// This file was modified by the Torox Project.
// Drop iOS Platform support on the Rox-Engine
//
// This file incorporates code from the nya-engine project, which is licensed under the MIT License.
// See the LICENSE-MIT file in the root directory for more information.
//
// This file is also part of the Rox-engine, which is licensed under a dual-license system:
// 1. Free Use License (for non-commercial and commercial use under specific conditions)
// 2. Commercial License (for use on proprietary platforms)
// See the LICENSE file in the root directory for the full Rox-engine license terms.
// could be extended externally at runtime via method_exchangeImplementations
// to implement necessary behavior

#pragma once

#if defined __APPLE__
#include "TargetConditionals.h"

#include <Cocoa/Cocoa.h>
#import <QuartzCore/CAMetalLayer.h>

@interface shared_app_delegate : NSObject <NSApplicationDelegate>
{
    rox_system::app *m_app;
    int m_antialiasing;
}

-(id)init_with_responder:(rox_system::app*)responder antialiasing:(int)aa;
@end

@interface app_view : NSView<NSWindowDelegate>
{
    CAMetalLayer *metal_layer;
    id <MTLTexture>  metal_depth;
    id <MTLTexture>  metal_stencil;
    id <MTLTexture>  metal_msaa;

    CVDisplayLinkRef _displayLink;
    dispatch_source_t _displaySource;

    rox_system::app *m_app;
    unsigned long m_time;
    int m_antialiasing;

    enum state
    {
        state_init,
        state_draw
    };

    state m_state;

    bool m_shift_pressed;
    bool m_ctrl_pressed;
    bool m_alt_pressed;
}
@end

#endif
