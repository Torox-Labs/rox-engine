//nya-engine (C) nyan.developer@gmail.com released under the MIT license (see LICENSE)

// could be extended externally at runtime via method_exchangeImplementations
// to implement necessary behavior

#pragma once

#if defined __APPLE__
#include "TargetConditionals.h"
#if TARGET_OS_IPHONE || TARGET_IPHONE_SIMULATOR

#import <UIKit/UIKit.h>
#import <OpenGLES/EAGL.h>
#import <OpenGLES/ES2/gl.h>
#import <OpenGLES/ES2/glext.h>
#import <QuartzCore/QuartzCore.h>

@interface view_controller : UIViewController<UIAccelerometerDelegate>
{
    CFTimeInterval m_time;

    EAGLContext *context;
    BOOL animating;
    BOOL is_background;
    float scale;
    CADisplayLink * __weak displayLink;
}

@property (readonly, nonatomic, getter=isAnimating) BOOL animating;
@property (readonly, nonatomic, getter=getScale) float scale;
@end

@interface shared_app_delegate : UIResponder <UIApplicationDelegate>
@property (strong, nonatomic) UIWindow *window;
@property (strong, nonatomic) view_controller *viewController;
@end

#else

#include <Cocoa/Cocoa.h>
#import <QuartzCore/CAMetalLayer.h>

@interface shared_app_delegate : NSObject <NSApplicationDelegate>
{
    nya_system::app *m_app;
    int m_antialiasing;
}

-(id)init_with_responder:(nya_system::app*)responder antialiasing:(int)aa;
@end

@interface app_view : NSView<NSWindowDelegate>
{
    NSOpenGLContext *gl_context;
    CAMetalLayer *metal_layer;
    id <MTLTexture>  metal_depth;
    id <MTLTexture>  metal_stencil;
    id <MTLTexture>  metal_msaa;

    CVDisplayLinkRef _displayLink;
    dispatch_source_t _displaySource;

    nya_system::app *m_app;
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
#endif
