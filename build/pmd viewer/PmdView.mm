//nya-engine (C) nyan.developer@gmail.com released under the MIT license (see LICENSE)

#import "PmdView.h"
#import "PmdDocument.h"

#include "load_vmd.h"
#include "load_xps.h"
#include "load_tdcg.h"
#include "load_x.h"
#include "texture_bgra_bmp.h"
#include <sys/stat.h>

#include "scene/camera.h"
#include "system/system.h"
#include "render/bitmap.h"
#include "resources/file_resources_provider.h"
#include "resources/composite_resources_provider.h"

#include <OpenGL/gl3.h>

void bps16to8(void *data,int width,int height,int channels)
{
    const unsigned char *from=(unsigned char *)data+1;
    unsigned char *to=(unsigned char *)data;

    for(int i=0;i<width*height*channels;++i)
        to[i]=from[i+i];
}

bool load_texture(nya_scene::shared_texture &res,nya_scene::resource_data &texture_data,const char* name)
{
    if(!texture_data.get_size())
        return false;

    NSData *data=[NSData dataWithBytesNoCopy:texture_data.get_data() 
                                          length: texture_data.get_size() freeWhenDone:NO];
    if(data==nil)
    {
        nya_log::log()<<"unable to load texture: NSData error\n";
        return false;
    }

    NSBitmapImageRep *image=[NSBitmapImageRep imageRepWithData:data];
    if(image==nil)
    {
        nya_log::log()<<"unable to load texture: invalid file\n";
        return false;
    }

    const unsigned int bpp=(unsigned int)[image bitsPerPixel];
    const unsigned int bps=(unsigned int)[image bitsPerSample];
    if(!bpp || !bps)
    {
        nya_log::log()<<"unable to load texture: unsupported format\n";
        return false;
    }

    const unsigned int channels=bpp/bps;

    nya_render::texture::color_format format;
    switch (channels)
    {
        case 1: format=nya_render::texture::greyscale; break;
        case 3: format=nya_render::texture::color_rgb; break;
        case 4: format=nya_render::texture::color_rgba; break;

        default: nya_log::log()<<"unable to load texture: unsupported format\n"; return false;
    }

    unsigned int width=(unsigned int)[image pixelsWide];
    unsigned int height=(unsigned int)[image pixelsHigh];

    unsigned char *image_data=[image bitmapData];

    if(bps==16)
        bps16to8(image_data,width,height,channels);

    nya_render::bitmap_flip_vertical(image_data,width,height,channels);

    res.tex.build_texture(image_data,width,height,format,-1);

    return true;
}

class shared_context
{
public:
    NSOpenGLContext *allocate()
    {
        if(!m_context)
        {
            NSOpenGLPixelFormatAttribute pixelFormatAttributes[] =
            {
                NSOpenGLPFAAccelerated,
                NSOpenGLPFADoubleBuffer,
                NSOpenGLPFADepthSize, 32,
                NSOpenGLPFAStencilSize, 8,
                NSOpenGLPFASampleBuffers,1,NSOpenGLPFASamples,2,
                NSOpenGLPFAOpenGLProfile, NSOpenGLProfileVersion3_2Core,
                0
            };

            NSOpenGLPixelFormat *pixelFormat=[[[NSOpenGLPixelFormat alloc] initWithAttributes:pixelFormatAttributes] autorelease];

            m_context=[[NSOpenGLContext alloc] initWithFormat:pixelFormat shareContext:nil];
        }

        ++m_ref_count;
        return m_context;
    }

    void free()
    {
        --m_ref_count;
        if(m_ref_count>0)
            return;

        if(m_context)
            [m_context release];

        m_context=0;
        m_ref_count=0;
    }

public:
    static shared_context &get()
    {
        static shared_context holder;
        return holder;
    }

    shared_context(): m_context(0),m_ref_count(0) {}

private:
    NSOpenGLContext *m_context;
    int m_ref_count;
};

@implementation PmdView

- (void)prepareOpenGL
{
    [super prepareOpenGL];
    int vsync=true;
    [[self openGLContext] setValues:&vsync forParameter:NSOpenGLCPSwapInterval];
    [[self openGLContext] makeCurrentContext];
}

-(id)initWithCoder:(NSCoder *)aDecoder
{
    NSOpenGLContext* openGLContext=shared_context::get().allocate();

    self=[super initWithCoder:aDecoder];
    if(self)
    {
        [self setOpenGLContext:openGLContext];
        [openGLContext makeCurrentContext];

        m_animation_timer=[NSTimer timerWithTimeInterval:1.0f/30 target:self
                                            selector:@selector(animate:) userInfo:nil repeats:YES];
        [[NSRunLoop currentRunLoop] addTimer:m_animation_timer forMode:NSDefaultRunLoopMode];
        [[NSRunLoop currentRunLoop] addTimer:m_animation_timer forMode:NSEventTrackingRunLoopMode];

        [self registerForDraggedTypes:[NSArray arrayWithObjects:NSFilenamesPboardType,nil]];
    }

    return self;
}

- (NSDragOperation)draggingEntered:(id<NSDraggingInfo>)sender { return NSDragOperationGeneric; }

- (BOOL)performDragOperation:(id<NSDraggingInfo>)sender
{
    NSArray *draggedFilenames=[[sender draggingPasteboard] propertyListForType:NSFilenamesPboardType];
    NSString *extension=[[[draggedFilenames objectAtIndex:0] pathExtension] lowercaseString];
    NSURL *url=[NSURL fileURLWithPath:[draggedFilenames objectAtIndex:0]];

    if([extension compare:@"vmd"]==NSOrderedSame || [extension compare:@"vpd"]==NSOrderedSame
       || [extension compare:@"pose"]==NSOrderedSame)
    {
        [self loadAnim:[url path].UTF8String];
        return YES;
    }

    NSArray *texFileTypes = [NSArray arrayWithObjects:@"spa",@"sph",@"tga",@"bmp",@"png",@"jpg",@"jpeg",@"dds",@"txt",nil];
    if([texFileTypes indexOfObject:extension]!=NSNotFound)
    {
        NSPoint pt = [self convertPoint:[sender draggingLocation] fromView:nil];
        m_mouse_old=[self convertPoint: pt fromView: nil];

        m_assign_name.assign([url path].UTF8String);
        m_pick_all=[[NSApp currentEvent] modifierFlags] & NSShiftKeyMask;

        if([extension compare:@"spa"]==NSOrderedSame)
            m_pick_mode=pick_assignspa;
        else if([extension compare:@"sph"]==NSOrderedSame)
            m_pick_mode=pick_assignsph;
        else if([extension compare:@"txt"]==NSOrderedSame)
            m_pick_mode=pick_assignmaterial;
        else if ([[[[url path] lastPathComponent] lowercaseString] hasPrefix:@"toon"])
            m_pick_mode=pick_assigntoon;
        else
            m_pick_mode=pick_assigntexture;

        [self setNeedsDisplay: YES];
        return YES;
    }

    return NO;
}

- (void) mouseDown: (NSEvent *) theEvent
{
    NSPoint pt=[theEvent locationInWindow];

    m_mouse_old=[self convertPoint: pt fromView: nil];

    const NSUInteger flags = [[NSApp currentEvent] modifierFlags];
    if(flags & NSCommandKeyMask && flags & NSAlternateKeyMask)
    {
        m_pick_mode=pick_showhide;
        [self setNeedsDisplay: YES];
    }
}

- (void) rightMouseDown: (NSEvent *) theEvent
{
    NSPoint pt=[theEvent locationInWindow];

    m_mouse_old=[self convertPoint: pt fromView: nil];

    const NSUInteger flags = [[NSApp currentEvent] modifierFlags];
    if(flags & NSCommandKeyMask && flags & NSAlternateKeyMask)
    {
        m_postprocess.show_groups.clear();
        [self setNeedsDisplay: YES];
    }
}
/*
- (void) mouseMoved:(NSEvent *)theEvent
{
    NSPoint pt=[theEvent locationInWindow];
    m_mouse_old=[self convertPoint: pt fromView: nil];
}
*/
- (void) mouseDragged: (NSEvent *) theEvent
{
    NSPoint pt=[theEvent locationInWindow];

    pt=[self convertPoint: pt fromView: nil];

    m_camera.add_rot(pt.x-m_mouse_old.x,-(pt.y-m_mouse_old.y));

    if([[NSApp currentEvent] modifierFlags] & NSCommandKeyMask)
        m_light_dir=nya_math::vec3::normalize(nya_scene::get_camera().get_pos());

    m_mouse_old=pt;

    if(!m_postprocess.mesh.get_anim().is_valid())
        [self setNeedsDisplay: YES];
}

- (void) rightMouseDragged: (NSEvent *) theEvent
{
    NSPoint pt=[theEvent locationInWindow];

    pt=[self convertPoint: pt fromView: nil];

    m_camera.add_pos((pt.x-m_mouse_old.x)/20.0f,(pt.y-m_mouse_old.y)/20.0f,0.0f);

    m_mouse_old=pt;

    if(!m_postprocess.mesh.get_anim().is_valid())
        [self setNeedsDisplay: YES];
}

- (void) scrollWheel: (NSEvent*) event
{
    m_camera.add_pos(0.0f,0.0f,[event deltaY]);

    if(!m_postprocess.mesh.get_anim().is_valid())
        [self setNeedsDisplay: YES];
}

-(void)reshape
{
    [super reshape];
    [[self openGLContext] update];

    const float scale=self.layer.contentsScale;

    unsigned int width=[self frame].size.width,height=[self frame].size.height;
    nya_render::set_viewport(0,0,width*scale,height*scale);
    m_camera.set_aspect(float(width)/height);

    [[self openGLContext] update];
    nya_render::apply_state(true);
    m_postprocess.resize(width*scale,height*scale);
    [self setNeedsDisplay: YES];
}

-(void)loadAnim:(const std::string &)name
{
    nya_scene::animation anim;
    nya_scene::animation::register_load_function(vmd_loader::load,true);
    nya_scene::animation::register_load_function(vmd_loader::load_pose,false);
    nya_scene::animation::register_load_function(xps_loader::load_pose,false);
    anim.load(name.c_str());
    m_postprocess.mesh.set_anim(anim);
    [m_anim_slider setEnabled:true];
    [m_anim_slider setIntValue:0];
    [m_anim_slider setMaxValue:anim.get_duration()];

    m_last_time=nya_system::get_time();
}

-(void)saveObj:(const std::string &)name
{
    const char *mesh_name=m_postprocess.mesh.get_name();
    if(!mesh_name || !mesh_name[0])
        return;

    const nya_scene::shared_mesh *sh=m_postprocess.mesh.internal().get_shared_data().operator->();
    if(!sh)
        return;

    const nya_render::vbo &vbo=sh->vbo;
    nya_memory::tmp_buffer_ref vert_buf;
    if(!vbo.get_vertex_data(vert_buf))
        return;

    nya_memory::tmp_buffer_ref inds_buf;
    if(!vbo.get_index_data(inds_buf))
        return;

    class obj_mesh
    {
    public:
        void add_vec(const char *id,const nya_math::vec3 &v)
        {
            char buf[512];
            sprintf(buf,"%s %f %f %f\n",id,v.x,v.y,v.z);
            m_data.append(buf);
        }

        void add_vec(const char *id,const nya_math::vec2 &v)
        {
            char buf[512];
            sprintf(buf,"%s %f %f\n",id,v.x,v.y);
            m_data.append(buf);
        }

        void add_face(int idx1,int idx2,int idx3)
        {
            ++idx1,++idx2,++idx3;
            char buf[512];
            sprintf(buf,"f %d/%d/%d %d/%d/%d %d/%d/%d\n",idx1,idx1,idx1,
                                                         idx2,idx2,idx2,
                                                         idx3,idx3,idx3);
            m_data.append(buf);
        }

        void add_group(const char *group_name)
        {
            char buf[512];
            sprintf(buf,"\ng %s\n",group_name);
            m_data.append(buf);
        }

        void add_group(const char *group_name,const char *mat_name)
        {
            char buf[512];
            sprintf(buf,"\ng %s\nusemtl %s\n",group_name,mat_name);
            m_data.append(buf);
        }

        void add_material(const char *mat_name,const char *tex_name)
        {
            m_materials_data.append("\nnewmtl ");
            m_materials_data.append(mat_name);
            m_materials_data.append("\nillum 2\n"
                          "Kd 0.800000 0.800000 0.800000\n"
                          "Ka 0.200000 0.200000 0.200000\n"
                          "Ks 0.000000 0.000000 0.000000\n"
                          "Ke 0.000000 0.000000 0.000000\n"
                          "Ns 0.000000\n");

            if(tex_name)
            {
                m_materials_data.append("map_Kd ");
                m_materials_data.append(tex_name);
                m_materials_data.append("\n");
            }
        }

        bool write_file(const char *filename)
        {
            FILE *o=fopen(filename,"wb");
            if(!o)
                return false;

            if(!m_materials_data.empty())
            {
                std::string name(filename);
                std::string path;
                size_t sep=name.find_last_of("\\/");
                if(sep!=std::string::npos)
                {
                    path=name.substr(0,sep+1);
                    name=name.substr(sep+1,name.size()-sep-1);
                }
                size_t dot=name.find_last_of(".");
                if(dot!=std::string::npos)
                    name=name.substr(0,dot);
                
                name+=".mtl";

                FILE *om=fopen((path+name).c_str(),"wb");
                if(!om)
                    return false;

                fwrite(&m_materials_data[0],1,m_materials_data.length(),om);
                fclose(om);

                char buf[512];
                sprintf(buf,"mtllib %s\n\n",name.c_str());

                fwrite(buf,1,strlen(buf),o);
            }

            if(!m_data.empty())
                fwrite(&m_data[0],1,m_data.length(),o);

            fclose(o);
            return true;
        }

    private:
        std::string m_data;
        std::string m_materials_data;
    } obj;


    const char *vdata=(char *)vert_buf.get_data();
    for(unsigned int i=0;i<vbo.get_verts_count();++i)
    {
        obj.add_vec("v",nya_math::vec3((float *)&vdata[i*vbo.get_vert_stride()+vbo.get_vert_offset()]));
        obj.add_vec("vn",nya_math::vec3((float *)&vdata[i*vbo.get_vert_stride()+vbo.get_normals_offset()]));
        obj.add_vec("vt",nya_math::vec2((float *)&vdata[i*vbo.get_vert_stride()+vbo.get_tc_offset(0)]));
    }

    vert_buf.free();

    std::string tex_cut_path(mesh_name);
    size_t sep=tex_cut_path.find_last_of("\\/");
    if(sep!=std::string::npos)
        tex_cut_path=tex_cut_path.substr(0,sep+1);

    const unsigned short *inds=(const unsigned short *)inds_buf.get_data();
    for(int i=0;i<(int)sh->groups.size();++i)
    {
        const nya_scene::shared_mesh::group &g=sh->groups[i];
        if(m_postprocess.mesh.is_mmd() && g.name=="edge")
            continue;

        char group_name[255];
        sprintf(group_name,"mesh%0d",i);

        char mat_name[255];
        sprintf(mat_name,"Material%0d",i);

        nya_scene::texture_proxy t=sh->materials[g.material_idx].get_texture("diffuse");
        const char *tex_name=t.is_valid()?t->get_name():0;
        if(tex_name && strncmp(tex_name, tex_cut_path.c_str(), tex_cut_path.length())==0)
            tex_name+=tex_cut_path.length();

        obj.add_material(mat_name,tex_name);

        obj.add_group(group_name,mat_name);
        for(int j=g.offset;j<g.offset+g.count;j+=3)
        {
            if(m_postprocess.mesh.is_mmd())
                obj.add_face(inds[j],inds[j+2],inds[j+1]);
            else
                obj.add_face(inds[j],inds[j+1],inds[j+2]);
        }
    }

    inds_buf.free();

    obj.write_file(name.c_str());
}

- (void)animate:(id)sender
{
    PmdDocument *doc=[[[self window] windowController] document];
    if(!doc)
    {
        [m_animation_timer invalidate];
        return;
    }

    if(!doc->m_export_obj_name.empty())
    {
        [self saveObj: doc->m_export_obj_name];
        doc->m_export_obj_name.clear();
    }

    if(!doc->m_animation_name.empty())
    {
        [self loadAnim: doc->m_animation_name];
        doc->m_animation_name.clear();
    }

    if(m_backface_cull!=doc->m_backface_cull)
    {
        m_backface_cull=doc->m_backface_cull;
        [self setNeedsDisplay:YES];
    }

    if(m_show_bones!=doc->m_show_bones)
    {
        m_show_bones=doc->m_show_bones;
        [self setNeedsDisplay:YES];
    }

    if(m_postprocess.get_condition("ssao")!=doc->m_enable_ssao)
    {
        m_postprocess.set_condition("ssao",doc->m_enable_ssao);
        [self setNeedsDisplay:YES];
    }

    if(m_postprocess.mesh.get_anim().is_valid())
    {
        const unsigned long time=nya_system::get_time();
        const int dt=int(time-m_last_time);
        m_last_time=time;

        const int slider_value=[m_anim_slider intValue];
        if(m_postprocess.mesh.get_anim_time()==slider_value)
        {
            m_postprocess.mesh.update(dt);
#ifdef USE_BULLET
            m_phys.update_pre();
            m_phys_world.update(dt);
            m_phys.update_post();
#endif
            [m_anim_slider setIntValue:m_postprocess.mesh.get_anim_time()];
        }
        else
            m_postprocess.mesh.set_anim_time(slider_value);

        [self setNeedsDisplay:YES];
    }
}

- (void)draw
{
    PmdDocument *doc=[[[self window] windowController] document];
    if (!doc)
        return;
    
    if(!doc->m_model_name.empty())
    {
        static nya_resources::file_resources_provider *main_res=new nya_resources::file_resources_provider();
        main_res->set_folder("");
        m_path = doc->m_model_name;
        auto slash = m_path.rfind('/');
        if(slash != std::string::npos)
            m_path.resize(slash+1);
        static bool once=false;
        if(!once)
        {
            once=true;

            std::string res_path([[NSBundle mainBundle] resourcePath].UTF8String);

            nya_resources::composite_resources_provider *comp_res=new nya_resources::composite_resources_provider();
            nya_resources::file_resources_provider *toon_res=new nya_resources::file_resources_provider();
            toon_res->set_folder((res_path+"/toon/").c_str());
            comp_res->add_provider(toon_res);
            nya_resources::file_resources_provider *sh_res=new nya_resources::file_resources_provider();
            sh_res->set_folder((res_path+"/shaders/").c_str());
            comp_res->add_provider(sh_res);
            nya_resources::file_resources_provider *mat_res=new nya_resources::file_resources_provider();
            mat_res->set_folder((res_path+"/materials/").c_str());
            comp_res->add_provider(mat_res);
            nya_resources::file_resources_provider *res=new nya_resources::file_resources_provider();
            res->set_folder(res_path.c_str());
            comp_res->add_provider(res);
            comp_res->add_provider(main_res);

            nya_resources::set_resources_provider(comp_res);

            nya_scene::texture::register_load_function(nya_scene::texture::load_dds);
            nya_scene::texture::register_load_function(nya_scene::texture::load_tga);
            nya_scene::texture::register_load_function(nya_scene::load_texture_bgra_bmp);
            nya_scene::texture::register_load_function(load_texture);
            nya_scene::texture::set_load_dds_flip(true);
            nya_render::texture::set_default_aniso(4);

            nya_scene::mesh::register_load_function(pmd_loader::load,false);
            nya_scene::mesh::register_load_function(pmx_loader::load,false);
            nya_scene::mesh::register_load_function(tdcg_loader::load_hardsave,false);
            nya_scene::mesh::register_load_function(x_loader::load_text,false);
            nya_scene::mesh::register_load_function(xps_loader::load_mesh,false);
            nya_scene::mesh::register_load_function(xps_loader::load_mesh_ascii,false);
        }

        //nya_render::set_clear_color(1.0f,1.0f,1.0f,0.0f);
        nya_render::set_clear_color(0.2f,0.4f,0.5f,0.0f);

        m_light_dir=nya_math::vec3(-1.0,1.0,1.0).normalize();

        nya_render::depth_test::enable(nya_render::depth_test::less);

        m_postprocess.load("postprocess.txt");

        m_postprocess.mesh.load(doc->m_model_name.c_str());
        const bool is_tdcg = nya_resources::check_extension(doc->m_model_name.c_str(), ".png");
        if(!m_postprocess.mesh.is_mmd() && !is_tdcg)
            m_postprocess.mesh.set_scale(10.0);

#ifdef USE_BULLET
        m_phys_world.init();
        m_phys_world.get_world()->setDebugDrawer(&m_bullet_debug);
        m_bullet_debug.set_debug_mode(btIDebugDraw::DBG_DrawWireframe);
        m_bullet_debug.set_alpha(0.7f);
        m_phys.init(&m_postprocess.mesh,m_phys_world.get_world());
#endif

        nya_render::apply_state(true);

        doc->m_model_name.clear();
        doc->m_mesh=&m_postprocess.mesh;
        doc->m_view=self;

        [self reshape];
        return;
    }

    if(m_pick_mode!=pick_none)
    {
        unsigned int g=0;
        if(!m_pick_all)
        {
            glClearStencil(0);
            nya_render::clear(true,true);
            glClear(GL_STENCIL_BUFFER_BIT);
            glEnable(GL_STENCIL_TEST);
            glStencilOp(GL_KEEP,GL_KEEP,GL_REPLACE);

            for(int i=0;i<m_postprocess.mesh.get_groups_count();++i)
                glStencilFunc(GL_ALWAYS,i+1,-1), m_postprocess.mesh.draw_group(i);

            glReadPixels(m_mouse_old.x,m_mouse_old.y, 1, 1, GL_STENCIL_INDEX, GL_UNSIGNED_INT, &g);
            glDisable(GL_STENCIL_TEST);
        }

        for(int i=0;i<m_postprocess.mesh.get_groups_count();++i)
        {
            if(m_pick_all)
                g=i+1;
            else
                i=m_postprocess.mesh.get_groups_count();

            const char *group_name=m_postprocess.mesh.get_group_name(g-1);

            //printf("group %s %d\n",group_name,g-1);

            if(g>0 && (!group_name || strcmp(group_name,"edge")!=0))
            {
                --g;
                if(m_pick_mode==pick_showhide)
                {
                    m_postprocess.show_groups.resize(m_postprocess.mesh.get_groups_count(),true);
                    m_postprocess.show_groups[g]=!m_postprocess.show_groups[g];

                    const nya_scene::shared_mesh *sh=m_postprocess.mesh.internal().get_shared_data().operator->();
                    if(sh)
                    {
                        for(int i=g+1;i<int(sh->groups.size());++i)
                        {
                            if(sh->groups[i].offset==sh->groups[g].offset &&
                               sh->groups[i].count==sh->groups[g].count)
                                m_postprocess.show_groups[i]=m_postprocess.show_groups[g];
                        }
                    }
                }
                else if(!m_assign_name.empty())
                {
                    auto &m=m_postprocess.mesh.modify_material(g);

                    if(m_pick_mode==pick_assignmaterial)
                    {
                        nya_resources::resources_provider &prev_res=nya_resources::get_resources_provider();

                        size_t sep=m_assign_name.find_last_of("/\\");
                        if(sep!=std::string::npos)
                        {
                            std::string path=m_assign_name.substr(0,sep+1);
                            m_assign_name=m_assign_name.substr(sep+1);

                            static nya_resources::file_resources_provider fp;
                            fp.set_folder(path.c_str());
                            nya_resources::set_resources_provider(&fp);
                        }

                        nya_scene::material pm=m;
                        m.load(m_assign_name.c_str());
                        for(int i=0;i<pm.get_textures_count();++i)
                            m.set_texture(pm.get_texture_semantics(i),pm.get_texture(i));
                        for(int i=0;i<pm.get_params_count();++i)
                            m.set_param(m.get_param_idx(pm.get_param_name(i)),pm.get_param(i));

                        nya_resources::set_resources_provider(&prev_res);
                    }
                    else
                    {
                        nya_scene::texture t;
                        if(t.load(m_assign_name.c_str()))
                        {
                            if(m_pick_mode==pick_assigntexture)
                                m.set_texture("diffuse",t);
                            else if(m_pick_mode==pick_assignspa)
                            {
                                m.set_texture("env",t);
                                m.set_param("env param", 0.0f,1.0f);
                            }
                            else if(m_pick_mode==pick_assignsph)
                            {
                                m.set_texture("env",t);
                                m.set_param("env param", 1.0f,0.0f);
                            }
                            else if(m_pick_mode==pick_assigntoon)
                            {
                                nya_render::texture rtex=t.internal().get_shared_data()->tex;
                                rtex.set_wrap(nya_render::texture::wrap_clamp, nya_render::texture::wrap_clamp);
                                m.set_texture("toon",t);
                            }
                        }
                    }
                }
            }
        }

        m_pick_mode=pick_none;
        m_assign_name.clear();
        m_pick_all=false;
    }

    if (doc->m_reload_textures)
    {
        if (m_textures_last_modified.empty())
        {
            for (int i = 0; i < m_postprocess.mesh.get_groups_count(); ++i)
            {
                auto &m = m_postprocess.mesh.get_material(i);
                for (int j = 0; j < m.get_textures_count(); ++j)
                {
                    auto &t = m.get_texture(j);
                    auto name = t.is_valid() ? t->get_name() : 0;
                    struct stat info;
                    if (name && stat(name, &info) == 0)
                        m_textures_last_modified[name] = info.st_mtime;
                }
            }
        }
        else
        {
            for (auto &tex: m_textures_last_modified)
            {
                struct stat info;
                if (stat(tex.first.c_str(), &info) != 0)
                    continue;
                
                auto t = info.st_mtime;
                if (tex.second == t)
                    continue;
                
                printf("tex %s was modified!\n", tex.first.c_str());
                tex.second = t;
                
                nya_scene::scene_shared<nya_scene::shared_texture>::reload_resource(tex.first.c_str());
            }
        }
        
        doc->m_reload_textures = false;
    }
/*
    nya_render::state_override so=nya_render::get_state_override();
    so.override_cull_face=m_backface_cull;
    so.cull_face=false;
    nya_render::set_state_override(so);
*/
    xps_loader::set_light_dir(m_light_dir);

    m_postprocess.draw(0);

    if(m_show_bones)
    {
        nya_render::clear(false,true);
        m_dd.set_point_size(3.0f);
        m_dd.clear();
        m_dd.add_skeleton(m_postprocess.mesh.get_skeleton(),nya_math::vec4(0.0,1.0,1.0,0.5));
        m_dd.draw();
#ifdef USE_BULLET
        m_phys_world.get_world()->debugDrawWorld();
#endif
    }
}

void postprocess::draw_scene(const char *pass, const nya_scene::tags &t)
{
    if(!show_groups.empty())
    {
        if(mesh.has_pass("opaque") || mesh.has_pass("transparent_blend"))
        {
            for(int i=0;i<int(show_groups.size());++i) if(show_groups[i]) mesh.draw_group(i,"opaque");
            for(int i=0;i<int(show_groups.size());++i) if(show_groups[i]) mesh.draw_group(i,"transparent_clip");
            for(int i=0;i<int(show_groups.size());++i) if(show_groups[i]) mesh.draw_group(i,"transparent_blend");
        }
        else
            for(int i=0;i<int(show_groups.size());++i) if(show_groups[i]) mesh.draw_group(i);
    }
    else
    {
        if(mesh.has_pass("opaque") || mesh.has_pass("transparent_blend"))
        {
            mesh.draw("opaque");
            mesh.draw("transparent_clip");
            mesh.draw("transparent_blend");
        }
        else
            mesh.draw();
    }
}

- (void)drawRect:(NSRect)rect
{
    [[self openGLContext] makeCurrentContext];

	[self draw];

    [[ self openGLContext ] flushBuffer];
}

-(void) dealloc
{
    m_postprocess.mesh.unload();

#ifdef USE_BULLET
    m_phys.release();
    m_phys_world.release();
    m_bullet_debug.release();
#endif

    shared_context::get().free();

    [super dealloc];
}

@end

@implementation MorphsWindow

-(id)init
{
    if(self = [super init])
    {
        [NSBundle loadNibNamed:@"MorphsConfig" owner:self];
    }

    return self;
}

-(void) dealloc
{
    [m_window close];
    [super dealloc];
}

-(void)displayWindow:(mmd_mesh *)mesh view:(PmdView *)view
{
    m_mesh=mesh;
    m_view=view;
    if(![m_window isVisible])
    {
        [m_window setIsVisible:YES];
        [m_window orderFront:nil];
    }

    if(!m_mesh)
        return;

    m_morphs_override.resize(m_mesh->get_morphs_count());
    [self resetAll:self];
}

- (NSInteger)numberOfRowsInTableView:(NSTableView *)pTableViewObj
{
    if(!m_mesh)
        return 0;

    return m_mesh->get_morphs_count();
}

- (id)tableView:(NSTableView *)tableView objectValueForTableColumn:(NSTableColumn *)tableColumn row:(int)row
{
    if(!m_mesh)
        return nil;

    const char *name=m_mesh->get_morph_name(row);
    if(!name)
        return nil;

    if([[tableColumn identifier] isEqualToString:@"name"])
        return [NSString stringWithUTF8String:name];

    if([[tableColumn identifier] isEqualToString:@"value"])
        return [NSNumber numberWithFloat:m_mesh->get_morph(row)*100.0f];

    return [NSNumber numberWithBool:m_morphs_override[row]];
}

- (IBAction)sliderChanged:(id)sender
{
    const int idx=(int)[m_table selectedRow];
    if(idx<0 || idx>=m_morphs_override.size())
        return;

    NSSliderCell *sender_slider = (NSSliderCell *)sender;
    const float v=[sender_slider floatValue]/100.0f;

    const NSUInteger flags = [[NSApp currentEvent] modifierFlags];
    const float mult=flags & NSShiftKeyMask?10.0f:1.0f;

    m_mesh->set_morph(idx,v*mult,m_morphs_override[idx]);
    m_mesh->update(0);
    [m_view setNeedsDisplay: YES];
}

- (void)tableView:(NSTableView *)tableView setObjectValue:(id)value forTableColumn:(NSTableColumn *)column row:(NSInteger)row
{
    if(!m_mesh)
        return;

    if([[column identifier] isEqualToString:@"value"])
    {
        const NSUInteger flags = [[NSApp currentEvent] modifierFlags];
        const float mult=flags & NSShiftKeyMask?10.0f:1.0f;
        m_mesh->set_morph(int(row),mult*[value floatValue]/100.0f,m_morphs_override[row]);
    }

    if([[column identifier] isEqualToString:@"override"])
    {
        m_morphs_override[row]=[value boolValue];
        m_mesh->set_morph(int(row),m_mesh->get_morph(int(row)),m_morphs_override[row]);
    }

    m_mesh->update(0);
    [m_table reloadData];

    [m_view setNeedsDisplay: YES];
}

-(IBAction)resetAll:(id)sender
{
    const NSUInteger flags = [[NSApp currentEvent] modifierFlags];
    const float value=flags & NSShiftKeyMask?1.0f:0.0f;

    for(int i=0;i<int(m_morphs_override.size());++i)
    {
        m_morphs_override[i]=false;
        m_mesh->set_morph(i,value,false);
    }

    m_mesh->update(0);
    [m_view setNeedsDisplay: YES];
    [m_table reloadData];
}

@end
