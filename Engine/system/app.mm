// Copyright (C) 2024 Torox Project
// Portions Copyright (C)) 2013 nyan.developer@gmail.com (nya-engine)
//
// This file was modified by the Torox Project.
// Drop the Support of The OpenGL for the macOS Platform
// Reconfigure the macOS Platform to run only on Metal
//
// This file incorporates code from the nya-engine project, which is licensed under the MIT License.
// See the LICENSE-MIT file in the root directory for more information.
//
// This file is also part of the Rox-engine, which is licensed under a dual-license system:
// 1. Free Use License (for non-commercial and commercial use under specific conditions)
// 2. Commercial License (for use on proprietary platforms)
// See the LICENSE file in the root directory for the full Rox-engine license terms.

#include "app.h"
#include "app_internal.h"
#include "system.h"
#include "memory/tmp_buffer.h"
#include "render/render.h"
#include "render/render_metal.h"

#include "TargetConditionals.h"
#import <QuartzCore/CAMetalLayer.h>

#import <Metal/Metal.h>
#import <MetalKit/MetalKit.h>

#include <iostream>
#include <string>

//MARK: MACOS
#ifdef __APPLE__
namespace
{

class shared_app
{
private:
    void start(int x,int y,unsigned int w,unsigned int h,int antialiasing,bool fullscreen,rox_system::app &app)
    {
        if(m_window)
            return;
        
        NSRect viewRect=fullscreen?[[NSScreen mainScreen] frame]:NSMakeRect(x,y,w,h);
        
        NSUInteger style = NSWindowStyleMaskTitled | NSWindowStyleMaskResizable | NSWindowStyleMaskClosable;
        m_window = [[NSWindow alloc] initWithContentRect:viewRect styleMask:style backing:NSBackingStoreBuffered defer:NO];
        
        NSString *title_str=[NSString stringWithCString:m_title.c_str() encoding:NSUTF8StringEncoding];
        [m_window setTitle:title_str];
        [m_window setOpaque:YES];
        shared_app_delegate *delegate=[[shared_app_delegate alloc] init_with_responder:&app antialiasing:antialiasing];
        [[NSApplication sharedApplication] setDelegate:delegate];
        setup_menu();
        [m_window orderFrontRegardless];
        if(fullscreen)
        {
            [m_window setCollectionBehavior:NSWindowCollectionBehaviorFullScreenPrimary];
            [m_window toggleFullScreen:nil];
        }
        [NSApp run];
    }

public:
    void start_windowed(int x,int y,unsigned int w,unsigned int h,int antialiasing,rox_system::app &app)
    {
        start(x,y,w,h,antialiasing,false,app);
    }

    void start_fullscreen(unsigned int w,unsigned int h,int antialiasing,rox_system::app &app)
    {
        start(0,0,w,h,antialiasing,true,app);
    }

    void finish(rox_system::app &app)
    {
        if(!m_window)
            return;

        [m_window close];
    }

    void on_window_close(rox_system::app &app)
    {
        if(!m_window)
            return;

        app.on_free();
        m_window=0;
        [NSApp stop:0];
    }

    void set_title(const char *title)
    {
        if(!title)
            m_title.clear();
        else
            m_title.assign(title);

        if(!m_window)
            return;

        NSString *title_str=[NSString stringWithCString:m_title.c_str() encoding:NSUTF8StringEncoding];
        [m_window setTitle:title_str];
    }

    void set_virtual_keyboard(rox_system::virtual_keyboard_type type) {}

    void set_mouse_pos(int x,int y)
    {
        const NSRect r=[m_window frame];
        const NSDictionary* dd=[m_window deviceDescription];
        const CGDirectDisplayID display=(CGDirectDisplayID)[[dd objectForKey:@"NSScreenNumber"] intValue];
        const int height=(int)CGDisplayPixelsHigh(display);
        CGDisplayMoveCursorToPoint(display,CGPointMake(x+r.origin.x,height-(y+r.origin.y)));
    }

private:
    void setup_menu()
    {
        NSMenu *mainMenuBar;
        NSMenu *appMenu;
        NSMenuItem *menuItem;

        mainMenuBar=[[NSMenu alloc] init];

        appMenu=[[NSMenu alloc] initWithTitle:@"Nya engine"];
        menuItem=[appMenu addItemWithTitle:@"Quit" action:@selector(terminate:) keyEquivalent:@"q"];
        [menuItem setKeyEquivalentModifierMask:NSCommandKeyMask];

        menuItem=[[NSMenuItem alloc] init];
        [menuItem setSubmenu:appMenu];

        [mainMenuBar addItem:menuItem];

        [NSApp performSelector:@selector(setAppleMenu:) withObject:appMenu];
        [NSApp setMainMenu:mainMenuBar];
    }

public:
    static shared_app &get_app()
    {
        static shared_app app;
        return app;
    }

    static NSWindow *get_window()
    {
        return get_app().m_window;
    }

public:
    shared_app(): m_window(0), m_title("Nya engine") {}

private:
    NSWindow *m_window;
    std::string m_title;
};

}

@implementation app_view
{
@private
    BOOL m_need_reshape;
}

-(id)initWithFrame:(CGRect)frame antialiasing:(int)aa
{
    self=[super initWithFrame:frame];
    m_antialiasing=aa;
    return self;
}

- (void)setFrameSize:(NSSize)newSize
{
    [super setFrameSize:newSize];
    m_need_reshape=YES;
}

- (void)setBoundsSize:(NSSize)newSize
{
    [super setBoundsSize:newSize];
    m_need_reshape=YES;
}

- (void)viewDidChangeBackingProperties
{
    [super viewDidChangeBackingProperties];
    m_need_reshape=YES;
}

-(void)set_responder:(rox_system::app*)responder
{
    m_app=responder;
}

-(void)initView
{
    _displaySource = dispatch_source_create(DISPATCH_SOURCE_TYPE_DATA_ADD, 0, 0, dispatch_get_main_queue());
    __block app_view* weakSelf = self;
    dispatch_source_set_event_handler(_displaySource, ^(){ [weakSelf draw]; });
    dispatch_resume(_displaySource);

    CVDisplayLinkCreateWithActiveCGDisplays(&_displayLink);
    CVDisplayLinkSetOutputCallback(_displayLink,&dispatchDraw,(void*)_displaySource);
    CVDisplayLinkSetCurrentCGDisplay(_displayLink,CGMainDisplayID());
    CVDisplayLinkStart(_displayLink);

    [[self window] setAcceptsMouseMovedEvents:YES];
    [[self window] setDelegate: self];

    //[[NSRunLoop currentRunLoop] addTimer:m_animation_timer forMode:NSDefaultRunLoopMode];
    //[[NSRunLoop currentRunLoop] addTimer:m_animation_timer forMode:NSEventTrackingRunLoopMode];

    m_state=state_init;
}

static CVReturn dispatchDraw(CVDisplayLinkRef displayLink,
                                 const CVTimeStamp* now,
                                 const CVTimeStamp* outputTime,
                                 CVOptionFlags flagsIn,
                                 CVOptionFlags* flagsOut,
                                 void* displayLinkContext)
{
    dispatch_source_t source = (dispatch_source_t)displayLinkContext;
    dispatch_source_merge_data(source, 1);
    return kCVReturnSuccess;
}

- (void)draw
{
    if(m_need_reshape)
        [self reshape];

    if(metal_layer)
        nya_render::render_metal::start_frame([metal_layer nextDrawable],metal_depth);

    switch(m_state)
    {
        case state_init:
        {
            const bool had_splash=m_app->on_splash();
            m_app->on_init();
            m_time=rox_system::get_time();
            m_state=state_draw;
            if(had_splash)
                break;
        }

        case state_draw:
        {
            const unsigned long time=rox_system::get_time();
            m_app->on_frame((unsigned int)(time-m_time));
            m_time=time;
        }
        break;
    }

    if(metal_layer)
        nya_render::render_metal::end_frame();
}

- (BOOL)isOpaque { return YES; }

-(void)reshape 
{
    m_need_reshape=false;
    
    CGFloat backingScaleFactor = [[self window] backingScaleFactor];
    CGFloat widthInPixel = [self frame].size.width * backingScaleFactor;
    CGFloat heightInPixel = [self frame].size.height * backingScaleFactor;

//    std::cout <<"Metal Render API: " << nya_render::render_api_metal << std::endl;
//    std::cout <<"Render API: " << nya_render::get_render_api() << std::endl;

    if(nya_render::get_render_api()==nya_render::render_api_metal)
    {
        m_antialiasing=1;//ToDo

        if(!metal_layer)
        {
            [self setWantsLayer:YES];
            metal_layer=[CAMetalLayer layer];
            metal_layer.device=MTLCreateSystemDefaultDevice();
            metal_layer.pixelFormat=MTLPixelFormatBGRA8Unorm;
            metal_layer.framebufferOnly=YES;
            nya_render::render_metal::set_device(metal_layer.device);
            [self.layer addSublayer: metal_layer];
        }
        metal_layer.frame=self.layer.frame;
        metal_layer.drawableSize=[self frame].size;

        if(!metal_depth || metal_depth.width!=metal_layer.drawableSize.width || metal_depth.height!=metal_layer.drawableSize.height)
        {
            MTLTextureDescriptor* desc = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat: MTLPixelFormatDepth32Float_Stencil8
                                          width:metal_layer.drawableSize.width height: metal_layer.drawableSize.height mipmapped: NO];
            desc.textureType=m_antialiasing>1?MTLTextureType2DMultisample:MTLTextureType2D;
            desc.sampleCount=m_antialiasing>1?m_antialiasing:1;
            desc.usage=MTLTextureUsageUnknown;
            desc.storageMode=MTLStorageModePrivate;
            desc.resourceOptions=MTLResourceStorageModePrivate;
            metal_depth=[metal_layer.device newTextureWithDescriptor:desc];
        }
    }

//    nya_render::set_viewport(0,0,widthInPixel,heightInPixel);
    nya_render::set_viewport(0,0,[self frame].size.width,[self frame].size.height);
    if(m_app)
    m_app->on_resize([self frame].size.width,[self frame].size.height);

    [self setNeedsDisplay:YES];
}

//- (void)updateAppearance{
//    if(@available(macOS 10.14, *)){
//        NSAppearance *appearence = self.effectiveAppearance;
//    }
//}

- (void)update_mpos:(NSEvent *)event
{
    NSPoint pt=[event locationInWindow];
    pt=[self convertPoint:pt fromView:nil];

    m_app->on_mouse_move(pt.x,pt.y);
}

- (void)mouseMoved:(NSEvent *)event
{
    [self update_mpos:event];
}

- (void)mouseDragged:(NSEvent *)event
{
    [self update_mpos:event];
}

- (void)rightMouseDragged:(NSEvent *)event
{
    [self update_mpos:event];
}

- (void)mouseDown:(NSEvent *)event
{
    [self update_mpos:event];
    m_app->on_mouse_button(rox_system::mouse_left,true);
}

- (void)mouseUp:(NSEvent *)event
{
    m_app->on_mouse_button(rox_system::mouse_left,false);
}

- (void)rightMouseDown:(NSEvent *)event
{
    [self update_mpos:event];
    m_app->on_mouse_button(rox_system::mouse_right,true);
}

- (void)rightMouseUp:(NSEvent *)event
{
    m_app->on_mouse_button(rox_system::mouse_right,false);
}

- (void)scrollWheel:(NSEvent*)event
{
    m_app->on_mouse_scroll(0,[event deltaY]);
}

-(unsigned int)cocoaKeyToX11Keycode:(unichar)key_char
{
    if(key_char>='A' && key_char<='Z')
        return rox_system::key_a+key_char-'A';

    if(key_char>='a' && key_char<='z')
        return rox_system::key_a+key_char-'a';

    if(key_char>='1' && key_char<='9')
        return rox_system::key_1+key_char-'1';

    if(key_char>=NSF1FunctionKey && key_char<=NSF35FunctionKey)
        return rox_system::key_f1+key_char-NSF1FunctionKey;

    switch(key_char)
    {
        case NSLeftArrowFunctionKey: return rox_system::key_left;
        case NSRightArrowFunctionKey: return rox_system::key_right;
        case NSUpArrowFunctionKey: return rox_system::key_up;
        case NSDownArrowFunctionKey: return rox_system::key_down;

        case ' ': return rox_system::key_space;
        case '\r': return rox_system::key_return;
        case '\t': return rox_system::key_tab;
        case '0': return rox_system::key_0;
            
        case '[': return rox_system::key_bracket_left;
        case ']': return rox_system::key_bracket_right;
        case ',': return rox_system::key_comma;
        case '.': return rox_system::key_period;

        case 0x1b: return rox_system::key_escape;
        case 0x7f: return rox_system::key_backspace;

        case NSPageUpFunctionKey: return rox_system::key_page_up;
        case NSPageDownFunctionKey: return rox_system::key_page_down;
        case NSEndFunctionKey: return rox_system::key_end;
        case NSHomeFunctionKey: return rox_system::key_home;
        case NSInsertFunctionKey: return rox_system::key_insert;
        case NSDeleteFunctionKey: return rox_system::key_delete;

        default: break;
    };

    //printf("unknown key: \'%c\' %x\n",key_char,key_char);

    return 0;
}

-(void)keyDown:(NSEvent *)event
{
    NSString *chars=[event characters];
    for(int i=0;i<[chars length];++i)
        m_app->on_charcode([chars characterAtIndex:i],true,[event isARepeat]);

    if([event isARepeat])
        return;

    NSString *key=[event charactersIgnoringModifiers];
    if([key length]!=1)
        return;

    const unsigned int x11key=[self cocoaKeyToX11Keycode:[key characterAtIndex:0]];
    if(x11key)
        m_app->on_keyboard(x11key,true);
}

-(void)keyUp:(NSEvent *)event
{
    NSString *chars=[event characters];
    for(int i=0;i<[chars length];++i)
        m_app->on_charcode([chars characterAtIndex:i],false,false);

    NSString *key=[event charactersIgnoringModifiers];
    if([key length]!=1)
        return;

    const unsigned int x11key=[self cocoaKeyToX11Keycode:[key characterAtIndex:0]];
    if(x11key)
        return m_app->on_keyboard(x11key,false);
}

- (void)windowWillClose:(NSNotification *)notification
{
    if(!m_app)
        return;

    shared_app::get_app().on_window_close(*m_app);
    m_app=0;
}

- (void)windowWillMiniaturize:(NSNotification *)notification
{
    m_app->on_suspend();
}

- (void)windowDidDeminiaturize:(NSNotification *)notification
{
    m_app->on_restore();
}

-(void)flagsChanged:(NSEvent *)event
{
    //ToDo: caps, left/right alt, ctrl, shift - 4ex: [event modifierFlags] == 131330

    const bool shift_pressed = ([event modifierFlags] & NSShiftKeyMask) == NSShiftKeyMask;
    if(shift_pressed!=m_shift_pressed)
        m_app->on_keyboard(rox_system::key_shift,shift_pressed), m_shift_pressed=shift_pressed;

    const bool ctrl_pressed = ([event modifierFlags] & NSControlKeyMask) == NSControlKeyMask;
    if(ctrl_pressed!=m_ctrl_pressed)
        m_app->on_keyboard(rox_system::key_control,ctrl_pressed), m_ctrl_pressed=ctrl_pressed;

    const bool alt_pressed = ([event modifierFlags] & NSAlternateKeyMask) == NSAlternateKeyMask;
    if(alt_pressed!=m_alt_pressed)
        m_app->on_keyboard(rox_system::key_alt,alt_pressed), m_alt_pressed=alt_pressed;
}

@end

@implementation shared_app_delegate

-(id)init_with_responder:(rox_system::app*)responder  antialiasing:(int)aa
{
    self=[super init];
    if(!self)
        return self;

    m_app=responder;
    m_antialiasing=aa;

    NSWindow *window=shared_app::get_window();
    app_view *view = [[app_view alloc] initWithFrame:window.frame antialiasing:aa];
    [view set_responder:m_app];
    [window setContentView:view];
    [window makeFirstResponder:view];
    [window setAcceptsMouseMovedEvents:YES];
    [view reshape];
    [view initView];

    nya_render::apply_state(true);
    return self;
}

- (void)applicationWillTerminate:(NSNotification *)aNotification
{
    m_app->finish();
}

@end

#endif

namespace rox_system
{

void app::start_windowed(int x,int y,unsigned int w,unsigned int h,int antialiasing)
{
    shared_app::get_app().start_windowed(x,y,w,h,antialiasing,*this);
}

void app::start_fullscreen(unsigned int w,unsigned int h,int aa)
{
    shared_app::get_app().start_fullscreen(w,h,aa,*this);
}

void app::set_title(const char *title)
{
    shared_app::get_app().set_title(title);
}

void app::set_virtual_keyboard(virtual_keyboard_type type)
{
    shared_app::get_app().set_virtual_keyboard(type);
}

void app::set_mouse_pos(int x,int y)
{
    shared_app::get_app().set_mouse_pos(x,y);
}

void app::finish()
{
    shared_app::get_app().finish(*this);
}

}
