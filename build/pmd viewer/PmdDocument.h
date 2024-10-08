//nya-engine (C) nyan.developer@gmail.com released under the MIT license (see LICENSE)

#import <Cocoa/Cocoa.h>
#import "PmdView.h"
#include <string>

@interface PmdDocument : NSDocument//<NSValidatedUserInterfaceItem>
{
@public
    std::string m_model_name;
    std::string m_animation_name;
    std::string m_export_obj_name;

    mmd_mesh *m_mesh;
    PmdView *m_view;
    bool m_backface_cull;
    bool m_show_bones;
    bool m_enable_ssao;
    bool m_reload_textures;

@private
    MorphsWindow *m_morphs_window;
}

@end
