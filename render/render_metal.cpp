//nya-engine (C) nyan.developer@gmail.com released under the MIT license (see LICENSE)

#include "render_metal.h"
#include "render_objects.h"
#include "shader_code_parser.h"
#include "memory/mutex.h"
#include "memory/tmp_buffer.h"
#include "render/bitmap.h"
#include <queue>
#include <list>

#if __APPLE__ && __OBJC__
  #include "TargetConditionals.h"
  #if !TARGET_IPHONE_SIMULATOR
    #import <QuartzCore/CAMetalLayer.h>
  #endif
  #import <Metal/Metal.h>
  #include <atomic>
#endif

namespace nya_render
{

bool render_metal::is_available() const
{
#ifdef __APPLE__
    return true; //ToDo
#else
    return false;
#endif
}

#ifdef __APPLE__

namespace
{
    render_api_interface::state applied_state;
    bool ignore_cache = true;
    id<MTLDevice> device;

    id<MTLDevice> mtl_device()
    {
        if(!device)
            device=MTLCreateSystemDefaultDevice();

        return device;
    }

    id<MTLCommandQueue> command_queue;
    id<MTLCommandBuffer> command_buffer;
    id<MTLRenderCommandEncoder> command_encoder;

    class metal_buf
    {
        id<MTLBuffer> m_buf=nil;

    public:
        void create(size_t size)
        {
#if TARGET_OS_IOS
            m_buf=[mtl_device() newBufferWithLength:size options:MTLResourceStorageModeShared];
#else
            m_buf=[mtl_device() newBufferWithLength:size options:MTLResourceStorageModeManaged];
#endif
            [m_buf contents];
        }

        void write(const void *data,size_t offset,size_t size)
        {
            [m_buf contents];

            memcpy((char *)[m_buf contents]+offset,data,size);
#if !TARGET_OS_IOS
            [m_buf didModifyRange:NSMakeRange(offset,size)];
#endif
        }

        void free() { m_buf=nil; }

        id<MTLBuffer> get() { return m_buf; }
        bool is_valid() { return m_buf!=nil; }

        //metal_buf() {}
        //metal_buf(size_t size) { create(size); }
        //metal_buf(id<MTLBuffer> from): m_buf(from) {}
    };
}

void render_metal::set_device(id<MTLDevice> d) { device=d; }

//shader

namespace
{

struct uniform_offset { int vs=-1,ps=-1,count=0; bool is_vec3=false; };

struct shader_obj
{
    id<MTLFunction> vertex;
    id<MTLFunction> fragment;

    std::vector<int> attributes;

    std::vector<shader::uniform> uniforms;
    std::vector<uniform_offset> uniform_offsets;

    int mat_p=-1,mat_mv=-1,mat_mvp=-1;
    int buf_size_v=0,buf_size_p=0;

public:
    void release() {} //ToDo
};
render_objects<shader_obj> shaders;

struct shader_buf
{
    class queue
    {
        std::vector<float> m_buf;
        std::queue<metal_buf> m_free;
        nya_memory::mutex m_mutex;

    public:
        void write(const float *f,int offset,int count,bool is_vec3)
        {
            if(is_vec3)
            {
                const float *from=f;
                float *to=&m_buf[offset];
                for(int i=0;i<count/3;++i,from+=3,to+=4)
                    memcpy(to,from,3*sizeof(float));
            }
            else
                memcpy(&m_buf[offset],f,count*sizeof(float));
        }

        const void *get_data() const { return m_buf.data(); }
        size_t get_size() const { return m_buf.size()*sizeof(float); }

        metal_buf bind() //ToDo: use previous if unchanged
        {
            metal_buf result;

            m_mutex.lock();

    size_t size=m_free.size(); static size_t last=size; if(size>last) printf("longest shader buf queue: %ld\n", (last=size)); //ToDo

            if(!m_free.empty())
            {
                result=m_free.front();
                m_free.pop();
            }
            m_mutex.unlock();

            if(!result.is_valid())
                result.create(m_buf.size()*4);
            result.write(m_buf.data(),0,m_buf.size()*4);
            return result;
        }

        void unbind(metal_buf buf) //other thread
        {
            m_mutex.lock();
            m_free.push(buf);
            m_mutex.unlock();
        }

        queue(int size) { m_buf.resize(size); }
    };

    queue *vertex=0;
    queue *fragment=0;
    std::vector<uniform_offset> offsets;

public:
    void create(shader_obj &s)
    {
        offsets=s.uniform_offsets;
        if(s.buf_size_v>0) vertex=new queue(s.buf_size_v);
        if(s.buf_size_p>0) fragment=new queue(s.buf_size_p);
    }

    void release()
    {
        //ToDo: if in use, delete in unbind()
        //if(vertex) delete vertex;
        //if(fragment) delete fragment;

        vertex=fragment=0;
        offsets.clear();
    }
};
render_objects<shader_buf> shader_bufs;
}

int render_metal::create_shader(const char *vertex,const char *fragment)
{
    MTLCompileOptions *compile_options = [[MTLCompileOptions alloc] init];
    compile_options.fastMathEnabled = true;

    const int idx=shaders.add();
    shader_obj &s=shaders.get(idx);

    for(int i=0;i<2;++i)
    {
        NSError *error;
        shader_code_parser parser(i==0?vertex:fragment);
        if(!parser.convert_to_metal())
        {
            shaders.remove(idx);
            return -1;
        }

        id<MTLLibrary> lib = [mtl_device() newLibraryWithSource:[NSString stringWithUTF8String:parser.get_code()] options:compile_options error:&error];
        if(error)
            NSLog(@"%@",error.description);

        if(!lib)
        {
            shaders.remove(idx);
            return -1;
        }

        if(i==0)
        {
            for(int i=0;i<parser.get_attributes_count();++i)
            {
                const std::string &name=parser.get_attribute(i).name;
                if(name=="Vertex")
                    s.attributes.push_back(0);
                else if(name=="Normal")
                    s.attributes.push_back(1);
                else if(name=="Color")
                    s.attributes.push_back(2);
                else
                    s.attributes.push_back(parser.get_attribute(i).idx+3);
            }

            s.vertex=[lib newFunctionWithName:@"_nya_main"];
        }
        else
            s.fragment=[lib newFunctionWithName:@"_nya_main"];

        int off=0;
        for(int j=0;j<parser.get_uniforms_count();++j)
        {
            auto from=parser.get_uniform(j);
            shader::uniform to;
            to.name=from.name;
            to.type=(shader::uniform_type)from.type;
            to.array_size=from.array_size;

            if(i==0 && to.type==shader::uniform_mat4)
            {
                if(from.name=="_nya_ModelViewProjectionMatrix")
                {
                    s.mat_mvp=off,off+=16;
                    continue;
                }
                if(from.name=="_nya_ModelViewMatrix")
                {
                    s.mat_mv=off,off+=16;
                    continue;
                }
                if(from.name=="_nya_ProjectionMatrix")
                {
                    s.mat_p=off,off+=16;
                    continue;
                }
            }

            int idx=-1;
            for(int k=0;k<(int)s.uniforms.size();++k) { if(s.uniforms[k].name==from.name) { idx=k; break; } }
            if(idx<0) { idx=(int)s.uniforms.size(); s.uniforms.push_back(to); s.uniform_offsets.resize(s.uniforms.size()); }

            s.uniform_offsets[idx].count=to.array_size*(to.type==shader::uniform_mat4?16:4);
            if(to.type>=shader::uniform_sampler2d)
                s.uniform_offsets[idx].count=0;
            else if(i==0)
                s.uniform_offsets[idx].vs=off;
            else
                s.uniform_offsets[idx].ps=off;
            s.uniform_offsets[idx].is_vec3=s.uniforms[idx].type==shader::uniform_vec3;
            off+=s.uniform_offsets[idx].count;
        }

        if(i==0)
            s.buf_size_v=off;
        else
            s.buf_size_p=off;
    }

    return idx;
}

uint render_metal::get_uniforms_count(int shader)
{
    return (uint)shaders.get(shader).uniforms.size();
}

shader::uniform render_metal::get_uniform(int shader,int idx)
{
    return shaders.get(shader).uniforms[idx];
}

int render_metal::create_uniform_buffer(int shader)
{
    int idx=shader_bufs.add();
    shader_buf &b=shader_bufs.get(idx);
    b.create(shaders.get(shader));
    return idx;
}

void render_metal::set_uniform(int uniform_buffer,int idx,const float *buf,uint count)
{
    shader_buf &b=shader_bufs.get(uniform_buffer);
    const uniform_offset &off=b.offsets[idx];
    if(off.vs>=0) b.vertex->write(buf,off.vs,count,off.is_vec3);
    if(off.ps>=0) b.fragment->write(buf,off.ps,count,off.is_vec3);
}

void render_metal::remove_uniform_buffer(int uniform_buffer) { shader_bufs.remove(uniform_buffer); }

//vbo

namespace
{
    class buffer_queue
    {
    public:
        class qued_buf: public metal_buf, nya_memory::non_copyable
        {
            std::atomic_int m_ref_count;

        public:
            qued_buf(size_t size) { m_ref_count=0; create(size); }

            bool is_used() const { return m_ref_count>0; }
            void bind() { ++m_ref_count; }
            void unbind() { --m_ref_count; }
        };

    private:
        size_t m_size;
        std::list<qued_buf *> m_pool;
        qued_buf *m_current;
        nya_memory::mutex m_mutex;

    public:
        qued_buf *add()
        {
            qued_buf *result=0;

            m_mutex.lock();
            size_t size=m_pool.size(); static size_t last=size; if(size>last) printf("longest vert buf queue: %ld\n", (last=size)); //ToDo
            for(auto &b: m_pool)
            {
                if(!b->is_used())
                {
                    result=b;
                    break;
                }
            }
            if(!result)
            {
                m_pool.push_back(new qued_buf(m_size));
                result=m_pool.back();
            }
            m_mutex.unlock();

            return result;
        }

        void write(const void *data)
        {
            if(!m_current->is_used())
            {
                m_current->write(data,0,m_size);
                return;
            }

            m_current=add();
            m_current->write(data,0,m_size);
        }

        qued_buf *bind() { m_current->bind(); return m_current; }

        buffer_queue(size_t size): m_size(size) { m_current=add(); }
    };

    class vdesc_cache
    {
        struct vdesc
        {
            vbo::layout layout;
            unsigned int stride;

            static bool attribute_equal(vbo::layout::attribute a,vbo::layout::attribute b)
            {
                if(a.dimension!=b.dimension)
                    return false;

                if(a.dimension==0)
                    return true;

                return a.offset==b.offset && a.type==b.type;
            }

            bool is_equal(const vbo::layout &other,unsigned int other_stride)
            {
                if(!attribute_equal(layout.pos,other.pos)) return false;
                if(!attribute_equal(layout.color,other.color)) return false;
                if(!attribute_equal(layout.normal,other.normal)) return false;
                for(int i=0;i<vbo::max_tex_coord;++i)
                {
                    if(!attribute_equal(layout.tc[i],other.tc[i]))
                        return false;
                }
                return stride==other_stride;
            }
        };

        std::vector<vdesc> m_vdescs;

    public:
        int get_idx(const vbo::layout &layout,unsigned int stride)
        {
            for(unsigned int i=0;i<(unsigned int)m_vdescs.size();++i)
            {
                if(m_vdescs[i].is_equal(layout,stride))
                    return i;
            }

            vdesc d;
            d.layout=layout;
            d.stride=stride;
            m_vdescs.push_back(d);
            return (int)m_vdescs.size()-1;
        }

        const vbo::layout &get_layout(int idx) const { return m_vdescs[idx].layout; }
        unsigned int get_stride(int idx) const { return m_vdescs[idx].stride; }

    } vdescs;

    struct vert_buf
    {
        metal_buf buf;
        buffer_queue *queue;
        uint count;
        int vdesc;

        void create_queue() { queue=new buffer_queue(vdescs.get_stride(vdesc)*count); }

        vert_buf(): queue(0),count(0),vdesc(-1) {}

    public:
        void release() { if(queue) delete queue; *this=vert_buf(); }
    };
    render_objects<vert_buf> vert_bufs;

    struct ind_buf
    {
        metal_buf buffer;
        vbo::index_size type;

    public:
        void release() {} //ToDo
    };
    render_objects<ind_buf> ind_bufs;
}

int render_metal::create_vertex_buffer(const void *data,uint stride,uint count,vbo::usage_hint usage)
{
    const int idx=vert_bufs.add();
    vert_buf &v=vert_bufs.get(idx);
    v.count=count;
    v.buf.create(stride*count);
    v.buf.write(data,0,stride*count);
    v.vdesc=vdescs.get_idx(vbo::layout(),stride);
    return idx;
}

void render_metal::update_vertex_buffer(int idx,const void *data)
{
    vert_buf &v=vert_bufs.get(idx);
    if(!v.queue)
    {
        v.create_queue();
        v.buf.free();
    }
    v.queue->write(data);
}

void render_metal::set_vertex_layout(int idx,vbo::layout layout)
{
    vert_buf &v=vert_bufs.get(idx);
    v.vdesc=vdescs.get_idx(layout,vdescs.get_stride(v.vdesc));
}

void render_metal::remove_vertex_buffer(int idx)
{
    vert_bufs.remove(idx);
    if(applied_state.vertex_buffer==idx)
        applied_state.vertex_buffer=-1;
}

int render_metal::create_index_buffer(const void *data,vbo::index_size type,uint count,vbo::usage_hint usage)
{
    const int idx=ind_bufs.add();
    ind_buf &i=ind_bufs.get(idx);
    i.buffer.create(type*count);
    i.buffer.write(data,0,type*count);
    i.type=type;
    return idx;
}

void render_metal::remove_index_buffer(int idx)
{
    ind_bufs.remove(idx);
    if(applied_state.index_buffer==idx)
        applied_state.index_buffer=-1;
}

//textures

namespace
{
    struct tex_obj
    {
        id<MTLTexture> texture;
        id<MTLSamplerState> sampler;
        texture::color_format format;
        bool has_mip;

    public:
        void release()
        {
            texture=nil;
            sampler=nil;
        }
    };
    render_objects<tex_obj> textures;

    MTLPixelFormat get_pixel_format(texture::color_format f)
    {
        switch(f)
        {
            case texture::color_rgba: return MTLPixelFormatRGBA8Unorm;
            case texture::color_bgra: return MTLPixelFormatBGRA8Unorm;

            default: break;
        }

        return MTLPixelFormatInvalid;
    }
}

int render_metal::create_texture(const void *data,uint width,uint height,texture::color_format &format,int mip_count)
{
    const int idx=textures.add();
    tex_obj &t=textures.get(idx);
    t.format=format;
    t.has_mip=(mip_count>1 || mip_count<0);

    MTLTextureDescriptor *td=[MTLTextureDescriptor texture2DDescriptorWithPixelFormat:get_pixel_format(format)
                                                                                width:width height:height mipmapped:t.has_mip];
    if (mip_count>0)
        td.mipmapLevelCount=mip_count;

    t.texture=[mtl_device() newTextureWithDescriptor:td];

    texture::wrap default_wrap_s,default_wrap_t;
    texture::get_default_wrap(default_wrap_s,default_wrap_t);
    const bool pot=((width&(width-1))==0 && (height&(height-1))==0);
    const bool force_clamp=!pot && !is_platform_restrictions_ignored();

    texture::filter default_min_filter,default_mag_filter,default_mip_filter;
    texture::get_default_filter(default_min_filter,default_mag_filter,default_mip_filter);

    MTLSamplerDescriptor *sd=[[MTLSamplerDescriptor alloc] init];
    sd.minFilter=default_min_filter==texture::filter_nearest?MTLSamplerMinMagFilterNearest:MTLSamplerMinMagFilterLinear;
    sd.magFilter=default_mag_filter==texture::filter_nearest?MTLSamplerMinMagFilterNearest:MTLSamplerMinMagFilterLinear;
    if (t.has_mip)
        sd.mipFilter=default_mag_filter==texture::filter_nearest?MTLSamplerMipFilterNearest:MTLSamplerMipFilterLinear;
    else
        sd.mipFilter=MTLSamplerMipFilterNotMipmapped;
    sd.maxAnisotropy=texture::get_default_aniso();
    if(sd.maxAnisotropy<1)
        sd.maxAnisotropy=1;
    sd.sAddressMode=force_clamp || default_wrap_s == texture::wrap_clamp ? MTLSamplerAddressModeClampToEdge : MTLSamplerAddressModeRepeat;
    sd.tAddressMode=force_clamp || default_wrap_t == texture::wrap_clamp ? MTLSamplerAddressModeClampToEdge : MTLSamplerAddressModeRepeat;
    sd.normalizedCoordinates=true;
    t.sampler=[mtl_device() newSamplerStateWithDescriptor:sd];

    if(!data)
        return idx;

    if(mip_count<0)
    {
        nya_memory::tmp_buffer_scoped buf((width*height*texture::get_format_bpp(format)/8)/2);
        for(int i=0;;++i,width=width>1?width/2:1,height=height>1?height/2:1)
        {
            const MTLRegion region={{0,0,0},{width,height,1}};
            const unsigned int bpr=width*texture::get_format_bpp(format)/8;
            [t.texture replaceRegion:region mipmapLevel:i withBytes:data bytesPerRow:bpr];
            if(width==1 && height==1)
                break;

            bitmap_downsample2x((unsigned char *)data,width,height,texture::get_format_bpp(format)/8,(unsigned char *)buf.get_data());
            data=buf.get_data();
        }

        //ToDo: generateMipmapsForTexture
    }
    else
    {
        for(int i=0;i<mip_count;++i,width=width>1?width/2:1,height=height>1?height/2:1)
        {
            const MTLRegion region={{0,0,0},{width,height,1}};
            const unsigned int bpr=width*texture::get_format_bpp(format)/8;
            [t.texture replaceRegion:region mipmapLevel:i withBytes:data bytesPerRow:bpr];
            data=(char *)data+bpr*height;
        }
    }

    return idx;
}

int render_metal::create_cubemap(const void *data[6],uint width,texture::color_format &format,int mip_count)
{
    return -1;
}

void render_metal::update_texture(int idx,const void *data,uint x,uint y,uint w,uint h,int mip)
{
    //ToDo: mipmaps

    tex_obj &t=textures.get(idx);
    MTLRegion region={{x,y,0},{w,h,1}};
    [t.texture replaceRegion:region mipmapLevel:mip>=0?mip:0 withBytes:data bytesPerRow:w*texture::get_format_bpp(t.format)/8];
}

bool render_metal::get_texture_data(int idx,uint x,uint y,uint w,uint h,void *data)
{
    tex_obj &t=textures.get(idx);
    MTLRegion region={{x,y,0},{w,h,1}};
    [t.texture getBytes:data bytesPerRow:w*texture::get_format_bpp(t.format)/8 fromRegion:region mipmapLevel:0];
    return true;
}

void render_metal::remove_texture(int texture) { textures.remove(texture); }

uint render_metal::get_max_texture_dimention()
{
#if TARGET_OS_IPHONE
    static int max_texture_size = 0;
    if (!max_texture_size)
    {
        if([mtl_device() supportsFeatureSet:MTLFeatureSet_iOS_GPUFamily3_v1])
           max_texture_size=16384;
        else if([mtl_device() supportsFeatureSet:MTLFeatureSet_iOS_GPUFamily1_v2] ||
                [mtl_device() supportsFeatureSet:MTLFeatureSet_iOS_GPUFamily2_v2])
            max_texture_size=8096;
        else
            max_texture_size=4096;
    }
    return max_texture_size;
#else
    return 16384;
#endif
}

bool render_metal::is_texture_format_supported(texture::color_format format)
{
    return get_pixel_format(format)!=MTLPixelFormatInvalid;
}

//general

namespace
{
#if !TARGET_IPHONE_SIMULATOR
    id<CAMetalDrawable> drawable;
#endif
    id<MTLTexture> depth_tex;
}

#if !TARGET_IPHONE_SIMULATOR
void render_metal::start_frame(id<CAMetalDrawable> _drawable,id<MTLTexture> depth)
{
    drawable=_drawable;
    depth_tex=depth;

    if(!command_queue)
    {
        command_queue=[mtl_device() newCommandQueue];
        command_queue.label=@"nya";
    }
    command_buffer=[command_queue commandBuffer];
}

void render_metal::end_frame()
{
    if(command_encoder)
    {
        [command_encoder endEncoding];
        command_encoder=nil;
    }

    if(drawable)
    {
        [command_buffer presentDrawable:drawable];
        [command_buffer commit];
        command_buffer=nil;
        drawable=nil;
    }
}
#endif

void render_metal::clear(const viewport_state &s,bool color,bool depth,bool stencil)
{
    MTLRenderPassDescriptor *rpd = [MTLRenderPassDescriptor renderPassDescriptor];

    if(s.target<0)
    {
#if !TARGET_IPHONE_SIMULATOR
        rpd.colorAttachments[0].texture=drawable.texture;
#endif
    }
    else
    {
        //ToDo
    }
    rpd.colorAttachments[0].storeAction=MTLStoreActionStore;
    if(color)
    {
        rpd.colorAttachments[0].loadAction=MTLLoadActionClear;
        rpd.colorAttachments[0].clearColor=MTLClearColorMake(s.clear_color[0],s.clear_color[1],s.clear_color[2],s.clear_color[3]);
    }

    rpd.depthAttachment.texture=depth_tex;
    rpd.depthAttachment.storeAction=MTLStoreActionDontCare;
    if(depth)
    {
        rpd.depthAttachment.loadAction=MTLLoadActionClear;
        rpd.depthAttachment.clearDepth=s.clear_depth;
    }

    rpd.stencilAttachment.texture=depth_tex;
    rpd.stencilAttachment.storeAction=MTLStoreActionDontCare;
    if(stencil)
    {
        rpd.stencilAttachment.loadAction=MTLLoadActionClear;
        rpd.stencilAttachment.clearStencil=s.clear_stencil;
    }

    if(command_encoder)
        [command_encoder endEncoding];
    command_encoder=[command_buffer renderCommandEncoderWithDescriptor:rpd];
    [command_encoder setViewport:(MTLViewport){(double)s.viewport.x,(double)s.viewport.y,
                                               (double)s.viewport.width,(double)s.viewport.height,-1.0, 1.0}];

    //ToDo: clamp bounds, disable scissor and viewport only if was set, calculate correct y with fbo
    if(s.scissor_enabled)
    {
        const int y=(int)drawable.texture.height-s.scissor.y-s.scissor.height;
        const MTLScissorRect r={(NSUInteger)s.scissor.x,(NSUInteger)y,(NSUInteger)s.scissor.width,(NSUInteger)s.scissor.height};
        [command_encoder setScissorRect:r];
    }
    else
    {
        const MTLScissorRect r={0,0,drawable.texture.width,drawable.texture.height};
        [command_encoder setScissorRect:r];
    }
}

namespace
{
    MTLBlendFactor get_blend_factor(blend::mode m)
    {
        switch(m)
        {
            case blend::zero: return MTLBlendFactorZero;
            case blend::one: return MTLBlendFactorOne;
            case blend::src_color: return MTLBlendFactorSourceColor;
            case blend::inv_src_color: return MTLBlendFactorOneMinusSourceColor;
            case blend::src_alpha: return MTLBlendFactorSourceAlpha;
            case blend::inv_src_alpha: return MTLBlendFactorOneMinusSourceAlpha;
            case blend::dst_color: return MTLBlendFactorDestinationColor;
            case blend::inv_dst_color: return MTLBlendFactorOneMinusDestinationColor;
            case blend::dst_alpha: return MTLBlendFactorDestinationAlpha;
            case blend::inv_dst_alpha: return MTLBlendFactorOneMinusDestinationAlpha;
        }
    }

    class pipeline_cache
    {
    public:
        struct desc
        {
            int shader;
            int target;
            int vdesc;

            bool blend;
            blend::mode blend_src;
            blend::mode blend_dst;

            bool operator < (const desc &other) const
            {
                if(vdesc<other.vdesc) return true;
                if(vdesc>other.vdesc) return false;

                if(shader<other.shader) return true;
                if(shader>other.shader) return false;

                if(blend!=other.blend) return other.blend;

                if(blend)
                {
                    if(blend_src<other.blend_src) return true;
                    if(blend_src>other.blend_src) return false;

                    if(blend_dst<other.blend_dst) return true;
                    if(blend_dst>other.blend_dst) return false;
                }

                return target<other.target;
            }
        };

        id<MTLRenderPipelineState> get(const desc &d)
        {
            auto it=m_cache.find(d);
            if(it!=m_cache.end())
                return it->second;

            MTLRenderPipelineDescriptor *pipeline_desc=[MTLRenderPipelineDescriptor new];
            pipeline_desc.colorAttachments[0].pixelFormat=drawable.texture.pixelFormat;
            pipeline_desc.depthAttachmentPixelFormat=depth_tex.pixelFormat;
            pipeline_desc.stencilAttachmentPixelFormat=depth_tex.pixelFormat;

            if(d.blend)
            {
                pipeline_desc.colorAttachments[0].blendingEnabled=YES;
                pipeline_desc.colorAttachments[0].sourceRGBBlendFactor=get_blend_factor(d.blend_src);
                pipeline_desc.colorAttachments[0].destinationRGBBlendFactor=get_blend_factor(d.blend_dst);
                pipeline_desc.colorAttachments[0].sourceAlphaBlendFactor=MTLBlendFactorOne;
                pipeline_desc.colorAttachments[0].destinationAlphaBlendFactor=MTLBlendFactorOne;
            }

            shader_obj &sh=shaders.get(d.shader);
            pipeline_desc.vertexFunction=sh.vertex;
            pipeline_desc.fragmentFunction=sh.fragment;

            const vbo::layout &l=vdescs.get_layout(d.vdesc);

            MTLVertexDescriptor *vd = [MTLVertexDescriptor vertexDescriptor];
            for(int i=0;i<(int)sh.attributes.size();++i)
            {
                const int idx=sh.attributes[i];
                const vbo::layout::attribute *a;
                switch(idx)
                {
                    case 0: a=&l.pos; break;
                    case 1: a=&l.normal; break;
                    case 2: a=&l.color; break;
                    default: a=&l.tc[idx-3]; break;
                }

                vd.attributes[idx].bufferIndex=0;

                if(!a->dimension)
                {
                    vd.attributes[idx].format=MTLVertexFormatFloat;
                    vd.attributes[idx].offset=0;
                    continue;
                }
                vd.attributes[idx].offset=a->offset;
                switch(a->type)
                {
                    case vbo::float32: vd.attributes[idx].format=MTLVertexFormat(MTLVertexFormatFloat+a->dimension-1); break;
                    case vbo::float16: vd.attributes[idx].format=MTLVertexFormat(MTLVertexFormatHalf2+(a->dimension>1?a->dimension:2)-2); break;
                    case vbo::uint8: vd.attributes[idx].format=MTLVertexFormat(MTLVertexFormatUChar2+(a->dimension>1?a->dimension:2)-2); break;
                }
            }
            vd.layouts[0].stride=vdescs.get_stride(d.vdesc);
            vd.layouts[0].stepFunction=MTLVertexStepFunctionPerVertex;
            pipeline_desc.vertexDescriptor=vd;

            NSError *error;
            auto pipeline=[mtl_device() newRenderPipelineStateWithDescriptor:pipeline_desc error:&error];
            if (!pipeline)
            {
                NSLog(@"Error occurred when creating render pipeline state: %@", error);
                return nil;
            }

            printf("created pipeline %p\n", pipeline);

            m_cache[d]=pipeline;
            return pipeline;
        }

        void remove_shader(int idx)
        {
            for(auto it=m_cache.begin();it!=m_cache.end();)
            {
                if (it->first.shader==idx)
                    m_cache.erase(it++);
                else
                    ++it;
            }
        }

        void remove_target(int idx)
        {
            for(auto it=m_cache.begin();it!=m_cache.end();)
            {
                if (it->first.target==idx)
                    m_cache.erase(it++);
                else
                    ++it;
            }
        }

    private:
        std::map<desc,id<MTLRenderPipelineState> > m_cache;

    } pipelines;

    class depth_stencil_cache
    {
    public:
        struct desc
        {
            bool depth_test;
            depth_test::comparsion depth_comparsion;
            bool zwrite;

            bool operator < (const desc &other) const
            {
                if(depth_test!=other.depth_test) return other.depth_test;

                if(depth_test)
                {
                    if(depth_comparsion<other.depth_comparsion) return true;
                    if(depth_comparsion>other.depth_comparsion) return false;
                }

                if(zwrite!=other.zwrite) return other.zwrite;

                return false;
            }
        };

        id<MTLDepthStencilState> get(const desc &d)
        {
            auto it=m_cache.find(d);
            if(it!=m_cache.end())
                return it->second;

            MTLDepthStencilDescriptor *dsd = [MTLDepthStencilDescriptor new];
            dsd.depthWriteEnabled=d.zwrite;
            if(d.depth_test)
            {
                switch(d.depth_comparsion)
                {
                    case depth_test::never: dsd.depthCompareFunction=MTLCompareFunctionNever; break;
                    case depth_test::less: dsd.depthCompareFunction=MTLCompareFunctionLess; break;
                    case depth_test::greater: dsd.depthCompareFunction=MTLCompareFunctionGreater; break;
                    case depth_test::not_less: dsd.depthCompareFunction=MTLCompareFunctionGreaterEqual; break;
                    case depth_test::not_greater: dsd.depthCompareFunction=MTLCompareFunctionLessEqual; break;
                    case depth_test::allways: dsd.depthCompareFunction=MTLCompareFunctionAlways; break;
                    case depth_test::equal: dsd.depthCompareFunction=MTLCompareFunctionEqual; break;
                    case depth_test::not_equal: dsd.depthCompareFunction=MTLCompareFunctionNotEqual; break;
                };
            }
            else
                dsd.depthCompareFunction=MTLCompareFunctionAlways;
            auto dss=[mtl_device() newDepthStencilStateWithDescriptor:dsd];
            m_cache[d]=dss;
            return dss;
        }

    private:
        std::map<desc,id<MTLDepthStencilState> > m_cache;

    } ds_states;
}

void render_metal::remove_shader(int shader)
{
    shaders.remove(shader);
    pipelines.remove_shader(shader);
    if(applied_state.shader==shader)
        applied_state.shader=-1;
}

namespace { nya_math::mat4 modelview, projection; }

void render_metal::set_camera(const nya_math::mat4 &mv,const nya_math::mat4 &p)
{
    modelview=mv;
    projection=p;
}

void render_metal::render_metal::draw(const state &s)
{
    if (s.vertex_buffer<0 || s.shader<0)
        return;

    //ToDo: target
    if(!command_encoder)
    {
        MTLRenderPassDescriptor *rpd=[MTLRenderPassDescriptor renderPassDescriptor];
        rpd.colorAttachments[0].texture=drawable.texture;
        rpd.colorAttachments[0].storeAction=MTLStoreActionStore;
        rpd.depthAttachment.texture=depth_tex;
        rpd.depthAttachment.storeAction=MTLStoreActionDontCare;
        rpd.stencilAttachment.texture=depth_tex;
        rpd.stencilAttachment.storeAction=MTLStoreActionDontCare;
        command_encoder=[command_buffer renderCommandEncoderWithDescriptor:rpd];
    }

    [command_encoder setViewport:(MTLViewport){(double)s.viewport.x,(double)s.viewport.y,
                                               (double)s.viewport.width,(double)s.viewport.height,-1.0, 1.0}];
    if(s.scissor_enabled)
    {
        const int y=(int)drawable.texture.height-s.scissor.y-s.scissor.height;
        const MTLScissorRect r={(NSUInteger)s.scissor.x,(NSUInteger)y,(NSUInteger)s.scissor.width,(NSUInteger)s.scissor.height};
        [command_encoder setScissorRect:r];
    }
    else
    {
        const MTLScissorRect r={0,0,drawable.texture.width,drawable.texture.height};
        [command_encoder setScissorRect:r];
    }

    vert_buf &vb=vert_bufs.get(s.vertex_buffer);

    pipeline_cache::desc d;
    d.shader=s.shader;
    d.target=s.target;
    d.vdesc=vb.vdesc;
    d.blend=s.blend;
    d.blend_src=s.blend_src;
    d.blend_dst=s.blend_dst;

    id<MTLRenderPipelineState> pipeline=pipelines.get(d);
    if(!pipeline)
        return;

    shader_obj &sh=shaders.get(s.shader);

    [command_encoder setRenderPipelineState:pipeline];

    [command_encoder setCullMode:s.cull_face?(s.cull_order==cull_face::ccw?MTLCullModeFront:MTLCullModeBack):MTLCullModeNone];

    //if(s.depth_test || s.zwrite)
    {
        depth_stencil_cache::desc d;
        d.depth_test=s.depth_test;
        d.depth_comparsion=s.depth_comparsion;
        d.zwrite=s.zwrite;
        [command_encoder setDepthStencilState:ds_states.get(d)];
    }

    if(vb.queue)
    {
        buffer_queue::qued_buf *vbuf=vb.queue->bind();
        [command_encoder setVertexBuffer:vbuf->get() offset:0 atIndex:0];
        [command_buffer addCompletedHandler:^(id<MTLCommandBuffer> commandBuffer) { vbuf->unbind(); }];
    }
    else
        [command_encoder setVertexBuffer:vb.buf.get() offset:0 atIndex:0];

    if(s.uniform_buffer>=0)
    {
        shader_buf &b=shader_bufs.get(s.uniform_buffer);
        shader_buf::queue *vq=b.vertex,*fq=b.fragment;
        if(sh.mat_mvp>=0)
        {
            const nya_math::mat4 mvp=modelview*projection;
            vq->write(mvp[0],sh.mat_mvp,16,false);
        }
        if(sh.mat_mv>=0) vq->write(modelview[0],sh.mat_mv,16,false);
        if(sh.mat_p>=0) vq->write(projection[0],sh.mat_p,16,false);

        const int small_buf_size=4096;
        metal_buf vb,fb;

        if(vq)
        {
            if(vq->get_size()<=small_buf_size)
            {
                [command_encoder setVertexBytes:vq->get_data() length:vq->get_size() atIndex:1];
                vq=0;
            }
            else
            {
                vb=vq->bind();
                [command_encoder setVertexBuffer:vb.get() offset:0 atIndex:1];
            }
        }

        if(fq)
        {
            if(fq->get_size()<=small_buf_size)
            {
                [command_encoder setFragmentBytes:fq->get_data() length:fq->get_size() atIndex:0];
                fq=0;
            }
            else
            {
                fb=fq->bind();
                [command_encoder setFragmentBuffer:fb.get() offset:0 atIndex:0];
            }
        }

        if(vq || fq)
            [command_buffer addCompletedHandler:^(id<MTLCommandBuffer> commandBuffer) { if(vq)vq->unbind(vb); if(fq)fq->unbind(fb); }];
    }

    for(int i=0;i<s.max_layers;++i)
    {
        if(s.textures[i]<0)
            continue;

        const tex_obj &t=textures.get(s.textures[i]);
        [command_encoder setFragmentTexture:t.texture atIndex:i];
        [command_encoder setFragmentSamplerState:t.sampler atIndex:i];
    }

    MTLPrimitiveType primitive;
    switch(s.primitive)
    {
        case vbo::triangles: primitive=MTLPrimitiveTypeTriangle; break;
        case vbo::triangle_strip: primitive=MTLPrimitiveTypeTriangleStrip; break;
        case vbo::points: primitive=MTLPrimitiveTypePoint; break;
        case vbo::lines: primitive=MTLPrimitiveTypeLine; break;
        case vbo::line_strip: primitive=MTLPrimitiveTypeLineStrip; break;
    }

    if(s.index_buffer>=0)
    {
        ind_buf inds=ind_bufs.get(s.index_buffer);
        [command_encoder drawIndexedPrimitives:primitive
                                    indexCount:s.index_count
                                     indexType:inds.type==vbo::index4b?MTLIndexTypeUInt32:MTLIndexTypeUInt16
                                   indexBuffer:inds.buffer.get()
                             indexBufferOffset:s.index_offset*inds.type];
    }
    else
        [command_encoder drawPrimitives:primitive vertexStart:s.index_offset vertexCount:s.index_count];
}

void render_metal::invalidate_cached_state() { ignore_cache = true; }

void render_metal::apply_state(const state &s)
{
    //ToDo?

    applied_state = s;
    ignore_cache = false;
}
#endif

render_metal &render_metal::get() { static render_metal *api = new render_metal(); return *api; }

}
