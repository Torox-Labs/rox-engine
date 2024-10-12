//nya-engine (C) nyan.developer@gmail.com released under the MIT license (see LICENSE)

#include "app.h"
#include "app_internal.h"
#include "system.h"
#include "memory/tmp_buffer.h"
#include "render/render.h"
#include "render/render_metal.h"

#include "TargetConditionals.h"
#if !TARGET_IPHONE_SIMULATOR
  #import <QuartzCore/CAMetalLayer.h>
#endif
#import <Metal/Metal.h>

#include <string>

#if TARGET_OS_IPHONE || TARGET_IPHONE_SIMULATOR
#import <UIKit/UIKit.h>

namespace
{
    class shared_app
    {
    public:
        void start_windowed(int x,int y,unsigned int w,unsigned int h,int antialiasing,nya_system::app &app)
        {
            start_fullscreen(w,h,antialiasing,app);
        }

        void start_fullscreen(unsigned int w,unsigned int h,int antialiasing,nya_system::app &app)
        {
            if(m_responder)
                return;

            m_responder=&app;
            m_antialiasing=antialiasing;

            @autoreleasepool 
            {
                UIApplicationMain(0,nil, nil, NSStringFromClass([shared_app_delegate class]));
            }
        }

        void finish(nya_system::app &app)
        {
            app.on_free();
            exit(0);
        }

        int get_touch_id(uintptr_t touch)
        {
            if(!touch)
                return -1;

            for(int i=0;i<(int)m_touches.size();++i)
            {
                if(m_touches[i]==touch)
                    return i;
            }

            return -1;
        }

        int add_touch_id(uintptr_t touch)
        {
            if(!touch)
                return -1;

            int touch_id=get_touch_id(touch);
            if(touch_id>=0)
                return touch_id;

            for(int i=0;i<(int)m_touches.size();++i)
            {
                if(m_touches[i])
                    continue;

                m_touches[i]=touch;
                return i;
            }

            m_touches.push_back(touch);
            return (int)m_touches.size()-1;
        }

        void remove_touch_id(uintptr_t touch)
        {
            for(size_t i=0;i<m_touches.size();++i)
            {
                if(m_touches[i]!=touch)
                    continue;

                m_touches[i]=0;
            }
        }

        void set_title(const char *title)
        {
            if(!title)
                m_title.clear();
            else
                m_title.assign(title);

            //ToDo: navigationItem.title
        }

        void set_virtual_keyboard(nya_system::virtual_keyboard_type type)
        {
            if(type==m_keyboard)
                return;

            m_keyboard=type;

            [[UIApplication sharedApplication].keyWindow.rootViewController.view resignFirstResponder];
            id<UITextInputTraits> traits=(id<UITextInputTraits>)UIApplication.sharedApplication.keyWindow.rootViewController.view;
            switch(type)
            {
                case nya_system::keyboard_hidden: return;

                case nya_system::keyboard_numeric: traits.keyboardType=UIKeyboardTypeDecimalPad; break;
                case nya_system::keyboard_decimal: traits.keyboardType=UIKeyboardTypeNumbersAndPunctuation; break;
                case nya_system::keyboard_phone: traits.keyboardType=UIKeyboardTypePhonePad; break;
                case nya_system::keyboard_text: traits.keyboardType=UIKeyboardTypeDefault; break;
                case nya_system::keyboard_pin: traits.keyboardType=UIKeyboardTypeNumberPad; break;
                case nya_system::keyboard_email: traits.keyboardType=UIKeyboardTypeEmailAddress; break;
                case nya_system::keyboard_password: traits.keyboardType=UIKeyboardTypeDefault; break;
                case nya_system::keyboard_url: traits.keyboardType=UIKeyboardTypeURL; break;
            }
            [[UIApplication sharedApplication].keyWindow.rootViewController.view becomeFirstResponder];
        }

        void set_mouse_pos(int x,int y) {}

    public:
        static shared_app &get_app() { static shared_app app; return app; }
        nya_system::app *get_responder() { return m_responder; }
        int get_antialiasing() { return m_antialiasing; }

    public:
        shared_app():m_responder(0),m_antialiasing(0),m_title("Nya engine"),m_keyboard(nya_system::keyboard_hidden) {}

    private:
        nya_system::app *m_responder;
        int m_antialiasing;
        std::vector<uintptr_t> m_touches;
        std::string m_title;
        nya_system::virtual_keyboard_type m_keyboard;
    };
}

@class EAGLContext;

@interface EAGLView : UIView<UIKeyInput, UITextInputTraits>
{
    GLint framebufferWidth;
    GLint framebufferHeight;
    float m_scale;

    GLuint defaultFramebuffer,colorRenderbuffer,msaaFramebuffer,msaaRenderBuffer,depthRenderbuffer;
}

@property (strong, nonatomic) EAGLContext *context;
@property (nonatomic) UIKeyboardType keyboardType;

- (void)setFramebuffer;
- (float)getScale;
- (BOOL)presentFramebuffer;

@end

@interface view_controller ()
@property (strong, nonatomic) EAGLContext *context;
@property (weak, nonatomic) CADisplayLink *displayLink;
@end

@implementation shared_app_delegate

@synthesize window = _window;
@synthesize viewController = _viewController;

- (BOOL)application:(UIApplication *)application didFinishLaunchingWithOptions:(NSDictionary *)launchOptions
{
    UIWindow *win=[[UIWindow alloc] initWithFrame:[[UIScreen mainScreen] bounds]];
    self.window = win;

    view_controller *vc=[[view_controller alloc] init];
    self.viewController = vc;

    [self.window setRootViewController:self.viewController];

    [self.window makeKeyAndVisible];

    [self.viewController.view setMultipleTouchEnabled:YES];

    return YES;
}

-(void)touch:(NSSet *)touches withEvent:(UIEvent *)event pressed:(BOOL)pressed button:(BOOL)button
{
    nya_system::app *responder=shared_app::get_app().get_responder();
    if(!responder)
        return;

    const float scale=[self.viewController getScale];
    for(UITouch *touch in touches)
    {
        int idx=shared_app::get_app().get_touch_id((uintptr_t)touch);
        if(pressed)
        {
            if(button)
            {
                if(idx>=0)
                    continue;

                idx=shared_app::get_app().add_touch_id((uintptr_t)touch);
            }
            else if(idx<0)
                continue;
        }
        else
        {
            if(idx<0)
                continue;

            shared_app::get_app().remove_touch_id((uintptr_t)touch);
        }

        const CGPoint tappedPt = [touch locationInView: self.viewController.view];
        const int x=tappedPt.x*scale;
        const int y=(self.viewController.view.bounds.size.height-tappedPt.y)*scale;

        responder->on_touch(x,y,idx,pressed);

        if(idx!=0)
            continue;

        responder->on_mouse_move(x,y);
        if(button)
            responder->on_mouse_button(nya_system::mouse_left,pressed);
    }
}

-(void)touchesBegan:(NSSet *)touches withEvent:(UIEvent *)event
{
    [self touch:touches withEvent:event pressed:true button:true];
};

-(void)touchesMoved:(NSSet *)touches withEvent:(UIEvent *)event
{
    [self touch:touches withEvent:event pressed:true button:false];
};

-(void)touchesEnded:(NSSet *)touches withEvent:(UIEvent *)event
{
    [self touch:touches withEvent:event pressed:false button:true];
};

-(void)touchesCancelled:(NSSet *)touches withEvent:(UIEvent *)event
{
    [self touch:touches withEvent:event pressed:false button:true];
};

@end


@implementation view_controller

@synthesize animating;
@synthesize scale;
@synthesize context;
@synthesize displayLink;

- (void)loadView 
{
    EAGLView *view=[[EAGLView alloc] initWithFrame:[UIScreen mainScreen].bounds];

    self.view = view;
    scale = [view getScale];
}

static inline NSString *NSStringFromUIInterfaceOrientation(UIInterfaceOrientation orientation) 
{
	switch (orientation) 
    {
		case UIInterfaceOrientationPortrait:           return @"UIInterfaceOrientationPortrait";
		case UIInterfaceOrientationPortraitUpsideDown: return @"UIInterfaceOrientationPortraitUpsideDown";
		case UIInterfaceOrientationLandscapeLeft:      return @"UIInterfaceOrientationLandscapeLeft";
		case UIInterfaceOrientationLandscapeRight:     return @"UIInterfaceOrientationLandscapeRight";
        default: break;
	}
	return @"Unexpected";
}

- (BOOL)shouldAutorotateToInterfaceOrientation:(UIInterfaceOrientation)interfaceOrientation 
{
    return [[[NSBundle mainBundle] objectForInfoDictionaryKey:@"UISupportedInterfaceOrientations"] indexOfObject:NSStringFromUIInterfaceOrientation(interfaceOrientation)] != NSNotFound;
}

//iOS 6:

- (BOOL)shouldAutorotate 
{
    return YES;
}
/*
- (NSUInteger)supportedInterfaceOrientations 
{
    return UIInterfaceOrientationMaskAll;
}
*/

- (void)viewDidLoad
{
    [super viewDidLoad];

    if(!self.context)
    {
        EAGLContext *aContext = [[EAGLContext alloc] initWithAPI:kEAGLRenderingAPIOpenGLES2];
        if(!aContext)
        {
            NSLog(@"Failed to create ES2 context");
            exit(0);
        }

        self.context=aContext;

        animating=NO;
        m_time=0.0;
        self.displayLink = nil;

        [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(applicationWillResignActive:) name:UIApplicationWillResignActiveNotification object:nil];
        [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(applicationDidBecomeActive:) name:UIApplicationDidBecomeActiveNotification object:nil];
        [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(applicationWillTerminate:) name:UIApplicationWillTerminateNotification object:nil];
        [[UIAccelerometer sharedAccelerometer]setDelegate:self];
    }

    [(EAGLView *)self.view setContext:context];
    [(EAGLView *)self.view setFramebuffer];

    nya_system::app *responder=shared_app::get_app().get_responder();
    if(responder)
    {
        [(EAGLView *)self.view setFramebuffer];
        if(responder->on_splash())
            [(EAGLView *)self.view presentFramebuffer];

        responder->on_init();
    }

    [self drawFrame];
}

- (void)accelerometer:(UIAccelerometer *)accelerometer didAccelerate:(UIAcceleration *)acceleration
{
    nya_system::app *responder=shared_app::get_app().get_responder();
    if(responder)
        responder->on_acceleration(acceleration.x,acceleration.y,acceleration.z);
}

- (void)applicationWillResignActive:(NSNotification *)notification
{
    nya_system::app *responder=shared_app::get_app().get_responder();
    if(responder)
        responder->on_suspend();

    if([self isViewLoaded])
        [self stopAnimation];

    is_background = true;
}

- (void)applicationDidBecomeActive:(NSNotification *)notification
{
    is_background = false;

    static bool ignore_first=true;
    if(!ignore_first)
    {
        nya_system::app *responder=shared_app::get_app().get_responder();
        if(responder)
            responder->on_restore();
    }
    else
        ignore_first=false;

    if([self isViewLoaded] && self.view.window)
        [self startAnimation];
}

- (void)applicationWillTerminate:(NSNotification *)notification
{
    if([self isViewLoaded])
        [self stopAnimation];
}

- (void)dealloc
{
    [[NSNotificationCenter defaultCenter] removeObserver:self];

    // Tear down context.
    if([EAGLContext currentContext] == context)
    {
        nya_system::app *responder=shared_app::get_app().get_responder();
        if(responder)
            responder->on_free();

        [EAGLContext setCurrentContext:nil];
    }
}

- (void)didReceiveMemoryWarning
{
    [super didReceiveMemoryWarning];
    // Release any cached data, images, etc. that aren't in use.

    nya_log::log()<<"app received memory warning, ";

    size_t tmp_buffers_size=nya_memory::tmp_buffers::get_total_size();
    nya_memory::tmp_buffers::force_free();
    nya_log::log()<<"forced to free "<<tmp_buffers_size-
                    nya_memory::tmp_buffers::get_total_size()<<" bytes\n";
}

- (void)viewWillAppear:(BOOL)animated
{
    if(!is_background)
        [self startAnimation];

    [super viewWillAppear:animated];
}

- (void)viewWillDisappear:(BOOL)animated
{
    if(!is_background)
        [self stopAnimation];

    [super viewWillDisappear:animated];
}

- (void)viewDidUnload
{
	[super viewDidUnload];

    // Tear down context.
    if([EAGLContext currentContext]==context)
        [EAGLContext setCurrentContext:nil];
	self.context = nil;	
}

- (void)startAnimation
{
    if(!animating)
    {
        CADisplayLink *aDisplayLink = [[UIScreen mainScreen] displayLinkWithTarget:self selector:@selector(drawFrame)];
        [aDisplayLink setFrameInterval:1];
        [aDisplayLink addToRunLoop:[NSRunLoop currentRunLoop] forMode:NSDefaultRunLoopMode];
        self.displayLink = aDisplayLink;

        animating = YES;
    }
}

- (void)stopAnimation
{
    if(animating)
    {
        [self.displayLink invalidate];
        self.displayLink = nil;
        animating = NO;
    }
}

- (void)drawFrame
{
    if(is_background)
        return;
    
    [(EAGLView *)self.view setFramebuffer];

    unsigned int dt=0;
    if(displayLink)
    {
        CFTimeInterval time=self.displayLink.timestamp;
        if(m_time)
            dt=(unsigned int)((time-m_time)*1000.0);
        m_time=time;
    }

    nya_system::app *responder=shared_app::get_app().get_responder();
    if(responder)
        responder->on_frame(dt);

    [(EAGLView *)self.view presentFramebuffer];
}

@end


@interface EAGLView (PrivateMethods)
- (void)createFramebuffer;
- (void)deleteFramebuffer;
@end

@implementation EAGLView

@synthesize context;

+ (Class)layerClass
{
    return [CAEAGLLayer class];
}

- (id)initWithFrame:(CGRect)frame 
{
    if((self=[super initWithFrame:frame]))
    {
        m_scale=1.0f;

        CAEAGLLayer *eaglLayer = (CAEAGLLayer *)self.layer;

        if([[UIScreen mainScreen] respondsToSelector:@selector(scale)])
            m_scale = eaglLayer.contentsScale = [[UIScreen mainScreen] scale];

        eaglLayer.opaque = TRUE;
        eaglLayer.drawableProperties = [NSDictionary dictionaryWithObjectsAndKeys:
                                        [NSNumber numberWithBool:FALSE], kEAGLDrawablePropertyRetainedBacking,
                                        kEAGLColorFormatRGBA8, kEAGLDrawablePropertyColorFormat,
                                        nil];
    }
    return self;
}

- (void)insertText:(NSString *)text
{
    nya_system::app *responder=shared_app::get_app().get_responder();

    for(int i=0;i<text.length;++i)
    {
        unichar c=[text characterAtIndex:i];
        responder->on_charcode(c,true,false);
    }
}

- (void)deleteBackward
{
    nya_system::app *responder=shared_app::get_app().get_responder();
    responder->on_charcode(nya_system::key_backspace,true,false);
}

- (BOOL)hasText { return YES; }
- (BOOL)canBecomeFirstResponder { return YES; }

- (void)dealloc
{
    [self deleteFramebuffer];
}

- (void)setContext:(EAGLContext *)newContext
{
    if(context!=newContext)
    {
        [self deleteFramebuffer];
        context=newContext;
        [EAGLContext setCurrentContext:nil];
    }
}

- (float)getScale { return m_scale; }

- (void)createFramebuffer
{
    if(context && !defaultFramebuffer)
    {
        [EAGLContext setCurrentContext:context];

        glGenFramebuffers(1, &defaultFramebuffer);
        glBindFramebuffer(GL_FRAMEBUFFER, defaultFramebuffer);

        glGenRenderbuffers(1, &colorRenderbuffer);
        glBindRenderbuffer(GL_RENDERBUFFER, colorRenderbuffer);

        glGenRenderbuffers(1, &depthRenderbuffer);
        glBindRenderbuffer(GL_RENDERBUFFER, depthRenderbuffer);

        glBindRenderbuffer(GL_RENDERBUFFER, colorRenderbuffer);

        if(![context renderbufferStorage:GL_RENDERBUFFER fromDrawable:(CAEAGLLayer *)self.layer])
        {
            NSLog(@"Unable to get renderbufferStorage");
            return;
        }

        glGetRenderbufferParameteriv(GL_RENDERBUFFER, GL_RENDERBUFFER_WIDTH, &framebufferWidth);
        glGetRenderbufferParameteriv(GL_RENDERBUFFER, GL_RENDERBUFFER_HEIGHT, &framebufferHeight);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, colorRenderbuffer);

        const int aa=shared_app::get_app().get_antialiasing();
        if(aa>1)
        {
            glGenFramebuffers(1, &msaaFramebuffer);
            glBindFramebuffer(GL_FRAMEBUFFER, msaaFramebuffer);

            glGenRenderbuffers(1, &msaaRenderBuffer);
            glBindRenderbuffer(GL_RENDERBUFFER, msaaRenderBuffer);

            glRenderbufferStorageMultisampleAPPLE(GL_RENDERBUFFER, aa, GL_RGB5_A1, framebufferWidth, framebufferHeight);
            glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, msaaRenderBuffer);
            glGenRenderbuffers(1, &depthRenderbuffer);

            glBindRenderbuffer(GL_RENDERBUFFER, depthRenderbuffer);
            glRenderbufferStorageMultisampleAPPLE(GL_RENDERBUFFER, aa, GL_DEPTH_COMPONENT16, framebufferWidth, framebufferHeight);
            glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depthRenderbuffer);
        }
        else
        {
            glBindRenderbuffer(GL_RENDERBUFFER, depthRenderbuffer);
            glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT16, framebufferWidth, framebufferHeight);
            //glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8_OES, framebufferWidth, framebufferHeight);

            glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depthRenderbuffer);
            //glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_RENDERBUFFER, depthRenderbuffer);
        }

        if(glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
            NSLog(@"Failed to make complete framebuffer object %x", glCheckFramebufferStatus(GL_FRAMEBUFFER));
    }
}

- (void)deleteFramebuffer
{
    if(!context)
        return;

    [EAGLContext setCurrentContext:context];

    if(msaaFramebuffer)
    {
        glDeleteFramebuffers(1,&msaaFramebuffer);
        msaaFramebuffer=0;
    }

    if(msaaRenderBuffer)
    {
        glDeleteRenderbuffers(1,&msaaRenderBuffer);
        msaaRenderBuffer=0;
    }

    if(defaultFramebuffer)
    {
        glDeleteFramebuffers(1,&defaultFramebuffer);
        defaultFramebuffer=0;
    }

    if(colorRenderbuffer)
    {
        glDeleteRenderbuffers(1,&colorRenderbuffer);
        colorRenderbuffer=0;
    }

    if(depthRenderbuffer)
    {
        glDeleteRenderbuffers(1,&depthRenderbuffer);
        depthRenderbuffer=0;
    }
}

- (void)setFramebuffer
{
    if(context)
    {
        [EAGLContext setCurrentContext:context];

        if(!defaultFramebuffer)
        {
            [self createFramebuffer];
            nya_system::app *responder=shared_app::get_app().get_responder();
            if(responder)
                responder->on_resize(framebufferWidth,framebufferHeight);
        }

        if(msaaFramebuffer)
            glBindFramebuffer(GL_FRAMEBUFFER, msaaFramebuffer);
        else
            glBindFramebuffer(GL_FRAMEBUFFER, defaultFramebuffer);

        nya_render::set_viewport(0,0,framebufferWidth,framebufferHeight);
    }
}

- (BOOL)presentFramebuffer
{
    if(!context)
        return false;

    [EAGLContext setCurrentContext:context];

    const GLenum attachments[]={GL_DEPTH_ATTACHMENT};
    glDiscardFramebufferEXT(GL_READ_FRAMEBUFFER_APPLE,1,attachments);

    if(msaaFramebuffer)
    {

        glBindFramebuffer(GL_READ_FRAMEBUFFER_APPLE,msaaFramebuffer);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER_APPLE,defaultFramebuffer);
        glResolveMultisampleFramebufferAPPLE();
    }

    glBindRenderbuffer(GL_RENDERBUFFER, colorRenderbuffer);

    //const GLenum attachments[] = { GL_DEPTH_ATTACHMENT, GL_COLOR_ATTACHMENT0 };
    //glDiscardFramebufferEXT(GL_FRAMEBUFFER , sizeof(attachments)/sizeof(GLenum), attachments);

    return [context presentRenderbuffer:GL_RENDERBUFFER];
}

- (void)layoutSubviews
{
    // The framebuffer will be re-created at the beginning of the next setFramebuffer method call.
    [self deleteFramebuffer];
}

@end

#else

#include <OpenGL/gl3.h>

#ifndef GL_MULTISAMPLE
    #define GL_MULTISAMPLE_ARB
#endif

namespace
{

class shared_app
{
private:
    void start(int x,int y,unsigned int w,unsigned int h,int antialiasing,bool fullscreen,nya_system::app &app)
    {
        if(m_window)
            return;

        NSRect viewRect=fullscreen?[[NSScreen mainScreen] frame]:NSMakeRect(x,y,w,h);
        m_window=[[NSWindow alloc] initWithContentRect:viewRect styleMask:NSTitledWindowMask|NSMiniaturizableWindowMask|NSResizableWindowMask|NSClosableWindowMask backing:NSBackingStoreBuffered defer:NO];

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
    void start_windowed(int x,int y,unsigned int w,unsigned int h,int antialiasing,nya_system::app &app)
    {
        start(x,y,w,h,antialiasing,false,app);
    }

    void start_fullscreen(unsigned int w,unsigned int h,int antialiasing,nya_system::app &app)
    {
        start(0,0,w,h,antialiasing,true,app);
    }

    void finish(nya_system::app &app)
    {
        if(!m_window)
            return;

        [m_window close];
    }

    void on_window_close(nya_system::app &app)
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

    void set_virtual_keyboard(nya_system::virtual_keyboard_type type) {}

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

-(void)set_responder:(nya_system::app*)responder
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
            m_time=nya_system::get_time();
            m_state=state_draw;
            if(had_splash)
                break;
        }

        case state_draw:
        {
            const unsigned long time=nya_system::get_time();
            m_app->on_frame((unsigned int)(time-m_time));
            m_time=time;
        }
        break;
    }

    if(gl_context)
        [gl_context flushBuffer];

    if(metal_layer)
        nya_render::render_metal::end_frame();
}

- (BOOL)isOpaque { return YES; }

-(void)reshape 
{
    m_need_reshape=false;

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
    else
    {
        if(!gl_context)
        {
            NSOpenGLPixelFormatAttribute attrs[] =
            {
                NSOpenGLPFADoubleBuffer,
                NSOpenGLPFADepthSize, 32,
                NSOpenGLPFAOpenGLProfile,NSOpenGLProfileVersion3_2Core,
                0
            };

            NSOpenGLPixelFormatAttribute attrs_aniso[] =
            {
                NSOpenGLPFADoubleBuffer,
                NSOpenGLPFADepthSize, 32,
                NSOpenGLPFASampleBuffers,1,NSOpenGLPFASamples,0,
                NSOpenGLPFAOpenGLProfile,NSOpenGLProfileVersion3_2Core,
                0
            };  attrs_aniso[6]=m_antialiasing;

            NSOpenGLPixelFormat *format=0;

            if(m_antialiasing>1)
                format=[[NSOpenGLPixelFormat alloc] initWithAttributes:attrs_aniso];
            else
                format=[[NSOpenGLPixelFormat alloc] initWithAttributes:attrs];

            gl_context=[[NSOpenGLContext alloc] initWithFormat:format shareContext:nil];
            [gl_context makeCurrentContext];
            [gl_context setView:self];

            if(m_antialiasing>1)
                glEnable(GL_MULTISAMPLE);
        }
        [gl_context update];
    }

    nya_render::set_viewport(0,0,[self frame].size.width,[self frame].size.height);
    if(m_app)
    m_app->on_resize([self frame].size.width,[self frame].size.height);

    [self setNeedsDisplay:YES];
}

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
    m_app->on_mouse_button(nya_system::mouse_left,true);
}

- (void)mouseUp:(NSEvent *)event
{
    m_app->on_mouse_button(nya_system::mouse_left,false);
}

- (void)rightMouseDown:(NSEvent *)event
{
    [self update_mpos:event];
    m_app->on_mouse_button(nya_system::mouse_right,true);
}

- (void)rightMouseUp:(NSEvent *)event
{
    m_app->on_mouse_button(nya_system::mouse_right,false);
}

- (void)scrollWheel:(NSEvent*)event
{
    m_app->on_mouse_scroll(0,[event deltaY]);
}

-(unsigned int)cocoaKeyToX11Keycode:(unichar)key_char
{
    if(key_char>='A' && key_char<='Z')
        return nya_system::key_a+key_char-'A';

    if(key_char>='a' && key_char<='z')
        return nya_system::key_a+key_char-'a';

    if(key_char>='1' && key_char<='9')
        return nya_system::key_1+key_char-'1';

    if(key_char>=NSF1FunctionKey && key_char<=NSF35FunctionKey)
        return nya_system::key_f1+key_char-NSF1FunctionKey;

    switch(key_char)
    {
        case NSLeftArrowFunctionKey: return nya_system::key_left;
        case NSRightArrowFunctionKey: return nya_system::key_right;
        case NSUpArrowFunctionKey: return nya_system::key_up;
        case NSDownArrowFunctionKey: return nya_system::key_down;

        case ' ': return nya_system::key_space;
        case '\r': return nya_system::key_return;
        case '\t': return nya_system::key_tab;
        case '0': return nya_system::key_0;
            
        case '[': return nya_system::key_bracket_left;
        case ']': return nya_system::key_bracket_right;
        case ',': return nya_system::key_comma;
        case '.': return nya_system::key_period;

        case 0x1b: return nya_system::key_escape;
        case 0x7f: return nya_system::key_backspace;

        case NSPageUpFunctionKey: return nya_system::key_page_up;
        case NSPageDownFunctionKey: return nya_system::key_page_down;
        case NSEndFunctionKey: return nya_system::key_end;
        case NSHomeFunctionKey: return nya_system::key_home;
        case NSInsertFunctionKey: return nya_system::key_insert;
        case NSDeleteFunctionKey: return nya_system::key_delete;

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
        m_app->on_keyboard(nya_system::key_shift,shift_pressed), m_shift_pressed=shift_pressed;

    const bool ctrl_pressed = ([event modifierFlags] & NSControlKeyMask) == NSControlKeyMask;
    if(ctrl_pressed!=m_ctrl_pressed)
        m_app->on_keyboard(nya_system::key_control,ctrl_pressed), m_ctrl_pressed=ctrl_pressed;

    const bool alt_pressed = ([event modifierFlags] & NSAlternateKeyMask) == NSAlternateKeyMask;
    if(alt_pressed!=m_alt_pressed)
        m_app->on_keyboard(nya_system::key_alt,alt_pressed), m_alt_pressed=alt_pressed;
}

@end

@implementation shared_app_delegate

-(id)init_with_responder:(nya_system::app*)responder  antialiasing:(int)aa
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

namespace nya_system
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
