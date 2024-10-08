//nya-engine (C) nyan.developer@gmail.com released under the MIT license (see LICENSE)

#import <Cocoa/Cocoa.h>

#include "mmd_mesh.h"

#ifdef USE_BULLET
    #include "mmd_phys.h"
    #include "extensions/bullet.h"
#endif

#include "scene/postprocess.h"
#include "render/debug_draw.h"
#include "viewer_camera.h"

struct postprocess: public nya_scene::postprocess
{
    mmd_mesh mesh;
    std::vector<bool> show_groups;

    void draw_scene(const char *pass,const nya_scene::tags &t);
};

@interface PmdView : NSOpenGLView
{
    NSPoint m_mouse_old;
    bool m_backface_cull;
    bool m_show_bones;

    viewer_camera m_camera;
    postprocess m_postprocess;
    nya_render::debug_draw m_dd;
    nya_math::vec3 m_light_dir;

#ifdef USE_BULLET
    mmd_phys_world m_phys_world;
    bullet_debug_draw m_bullet_debug;
    mmd_phys_controller m_phys;
#endif

    NSTimer *m_animation_timer;
    unsigned long m_last_time;

    IBOutlet NSSlider *m_anim_slider;

    enum pick_mode
    {
        pick_none,
        pick_showhide,
        pick_assigntexture,
        pick_assignspa,
        pick_assignsph,
        pick_assigntoon,
        pick_assignmaterial,

    } m_pick_mode;

    std::string m_assign_name;
    bool m_pick_all;
    
    std::string m_path;
    std::map<std::string, time_t> m_textures_last_modified;
}

@end

@interface MorphsWindow : NSObject
{
    IBOutlet NSWindow *m_window;
    IBOutlet NSTableView *m_table;
    mmd_mesh *m_mesh;
    PmdView *m_view;

    std::vector<bool> m_morphs_override;
}

-(void)displayWindow:(mmd_mesh *)mesh view:(PmdView *)view;

@end
