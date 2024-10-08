//nya-engine (C) nyan.developer@gmail.com released under the MIT license (see LICENSE)

#include "render_opengl.h"
#include "render_opengl_ext.h"
#include "memory/tmp_buffer.h"
#include "render_objects.h"
#include "bitmap.h"
#include "fbo.h"

namespace nya_render
{

bool render_opengl::is_available() const
{
    return true; //ToDo
}

namespace
{
    render_api_interface::state applied_state;
    bool ignore_cache = true;
	bool ignore_cache_vp = true;
    vbo::layout applied_layout;
    bool was_fbo_without_color=false;
    int default_fbo_idx=-1;

    enum
    {
        vertex_attribute=0,
        normal_attribute=1,
        color_attribute=2,
        tc0_attribute=3,
        max_attributes = tc0_attribute+16
    };

    int gl_cube_type(int side)
    {
        switch(side)
        {
            case fbo::cube_positive_x: return GL_TEXTURE_CUBE_MAP_POSITIVE_X;
            case fbo::cube_negative_x: return GL_TEXTURE_CUBE_MAP_NEGATIVE_X;
            case fbo::cube_positive_y: return GL_TEXTURE_CUBE_MAP_POSITIVE_Y;
            case fbo::cube_negative_y: return GL_TEXTURE_CUBE_MAP_NEGATIVE_Y;
            case fbo::cube_positive_z: return GL_TEXTURE_CUBE_MAP_POSITIVE_Z;
            case fbo::cube_negative_z: return GL_TEXTURE_CUBE_MAP_NEGATIVE_Z;
        }
        return GL_TEXTURE_2D;
    }

    void set_shader(int idx);

    struct shader_obj
    {
    public:
        shader_obj(): program(0)
        {
            memset(objects,0,sizeof(objects));
            mat_mvp=mat_mv=mat_p= -1;
        }

        GLuint program,objects[shader::program_types_count];

        struct uniform: public shader::uniform { int handler,cache_idx; };
        std::vector<uniform> uniforms;
        std::vector<float> uniform_cache;
        int mat_mvp,mat_mv,mat_p;

        void release()
        {
            for(int i=0;i<shader::program_types_count;++i)
            {
                if(!objects[i])
                    continue;

                glDetachShader(program,objects[i]);
                glDeleteShader(objects[i]);
            }

            if( program )
                glDeleteShader(program);

            *this=shader_obj();
        }
    };
    render_objects<shader_obj> shaders;

    void set_shader(int idx)
    {
        if(idx==applied_state.shader)
            return;

        if(idx<0)
        {
            glUseProgram(0);
            applied_state.shader= -1;
            return;
        }

        shader_obj &shdr=shaders.get(idx);
        glUseProgram(shdr.program);
        if(!shdr.program)
            applied_state.shader= -1;
        else
            applied_state.shader=idx;
    }

    int active_layer=-1;
    void gl_select_multitex_layer(int idx)
    {
        if(idx==active_layer)
            return;
        active_layer=idx;
        glActiveTexture(GL_TEXTURE0+idx);
    }
}

GLuint compile_shader(nya_render::shader::program_type type,const char *src)
{
    int gl_type;
    switch(type)
    {
        case nya_render::shader::vertex:   gl_type=GL_VERTEX_SHADER; break;
        case nya_render::shader::pixel:    gl_type=GL_FRAGMENT_SHADER; break;
        case nya_render::shader::geometry: gl_type=GL_GEOMETRY_SHADER; break;
        default: return 0;
    }

    GLuint shader=glCreateShader(gl_type);
    glShaderSource(shader,1,&src,0);
    glCompileShader(shader);
    GLint compiled=1;
    glGetShaderiv(shader,GL_COMPILE_STATUS,&compiled);
    if(!compiled)
    {
        GLint log_len=0;
        const static char type_str[][12]={"vertex","pixel","geometry","tesselation"};
        log()<<"Can't compile "<<type_str[type]<<" shader: \n";
        glGetShaderiv(shader,GL_INFO_LOG_LENGTH,&log_len);
        if(log_len>0)
        {
            std::string log_text(log_len,0);
            glGetShaderInfoLog(shader,log_len,&log_len,&log_text[0]);
            log()<<log_text.c_str()<<"\n";
        }
        return 0;
    }
    return shader;
}

int render_opengl::create_shader(const char *vertex,const char *fragment)
{
    init_extensions();

    const int idx=shaders.add();
    shader_obj &shdr=shaders.get(idx);

    shdr.program=glCreateProgram();
    if(!shdr.program)
    {
        log()<<"Unable to create shader program object\n";
        shaders.remove(idx);
        return -1;
    }

    std::vector<std::string> ft_vars;

    for(int i=0;i<2;++i)
    {
        shader_code_parser parser(i==0?vertex:fragment);
        shader::program_type type=i==0?shader::vertex:shader::pixel;

        if(i==0 && strstr(vertex,"gl_Position")==0)
        {
            for(int i=0;i<parser.get_out_count();++i)
                ft_vars.push_back(parser.get_out(i).name);
        }

  #ifdef OPENGL_ES
    #ifndef __APPLE__
        parser.fix_per_component_functions(); //some droids despise glsl specs
    #endif

        if(!parser.convert_to_glsl_es2())
        {
            log()<<"Unable to add shader program: cannot convert shader code to glsl for es2\n";
            log()<<parser.get_error()<<"\n";
            shaders.remove(idx);
            return -1;
        }
  #else
        if(!parser.convert_to_glsl3())
        {
            log()<<"Unable to add shader program: cannot convert shader code to glsl3\n";
            log()<<parser.get_error()<<"\n";
            shaders.remove(idx);
            return -1;
        }
  #endif
        GLuint object=compile_shader(type,parser.get_code());
        if(!object)
        {
            shaders.remove(idx);
            return -1;
        }

        glAttachShader(shdr.program,object);
        shdr.objects[type]=object;

        if(i==0)
        {
            for(int i=0;i<parser.get_attributes_count();++i)
            {
                const shader_code_parser::variable a=parser.get_attribute(i);
                if(a.name=="_nya_Vertex") glBindAttribLocation(shdr.program,vertex_attribute,a.name.c_str());
                else if(a.name=="_nya_Normal") glBindAttribLocation(shdr.program,normal_attribute,a.name.c_str());
                else if(a.name=="_nya_Color") glBindAttribLocation(shdr.program,color_attribute,a.name.c_str());
                else if(a.name.find("_nya_MultiTexCoord")==0) glBindAttribLocation(shdr.program,tc0_attribute+a.idx,a.name.c_str());
            }
        }

        for(int j=0;j<parser.get_uniforms_count();++j)
        {
            const shader_code_parser::variable from=parser.get_uniform(j);
            shader_obj::uniform to;
            to.name=from.name;
            to.type=(shader::uniform_type)from.type;
            to.array_size=from.array_size;

            if(to.type==shader::uniform_mat4)
            {
                if(to.name=="_nya_ModelViewProjectionMatrix")
                {
                    shdr.mat_mvp=1;
                    continue;
                }
                if(to.name=="_nya_ModelViewMatrix")
                {
                    shdr.mat_mv=1;
                    continue;
                }
                if(to.name=="_nya_ProjectionMatrix")
                {
                    shdr.mat_p=1;
                    continue;
                }
            }

            int idx=-1;
            for(int k=0;k<(int)shdr.uniforms.size();++k) { if(shdr.uniforms[k].name==from.name) { idx=k; break; } }
            if(idx<0) shdr.uniforms.push_back(to);
        }
    }

    if(!ft_vars.empty() && is_transform_feedback_supported())
    {
        std::vector<const GLchar *> vars;
        for(int i=0;i<(int)ft_vars.size();++i)
            vars.push_back(ft_vars[i].c_str());
        glTransformFeedbackVaryings(shdr.program,(GLsizei)vars.size(),vars.data(),GL_INTERLEAVED_ATTRIBS);
    }

    glLinkProgram(shdr.program);
    GLint result=1;
    glGetProgramiv(shdr.program,GL_LINK_STATUS,&result);
    if(!result)
    {
        log()<<"Can't link shader\n";
        GLint log_len=0;
        glGetProgramiv(shdr.program,GL_INFO_LOG_LENGTH,&log_len);
        if(log_len>0)
        {
            std::string log_text(log_len,0);
            glGetProgramInfoLog(shdr.program,log_len,&log_len,&log_text[0]);
            log()<<log_text.c_str()<<"\n";
        }

        shaders.remove(idx);
        return -1;
    }

    set_shader(idx);

    for(size_t i=0,layer=0;i<shdr.uniforms.size();++i)
    {
        const shader_obj::uniform &u=shdr.uniforms[i];
        if(u.type!=shader::uniform_sampler2d && u.type!=shader::uniform_sampler_cube)
            continue;

        int handler=glGetUniformLocation(shdr.program,u.name.c_str());
        if(handler>=0)
            glUniform1i(handler,(int)layer);
        else
            log()<<"Unable to set shader sampler \'"<<u.name.c_str()<<"\': probably not found\n";

        ++layer;
    }

    set_shader(-1);

#if !defined OPENGL_ES || defined __ANDROID__ //some android and desktop vendors ignore the standart
    set_shader(idx);
#endif

    set_shader(idx);

    if(shdr.mat_mvp>0)
        shdr.mat_mvp=glGetUniformLocation(shdr.program,"_nya_ModelViewProjectionMatrix");
    else if(shdr.mat_mv>0)
        shdr.mat_mv=glGetUniformLocation(shdr.program,"_nya_ModelViewMatrix");
    else if(shdr.mat_p>0)
        shdr.mat_p=glGetUniformLocation(shdr.program,"_nya_ProjectionMatrix");

    for(int i=0;i<(int)shdr.uniforms.size();++i)
        shdr.uniforms[i].handler=glGetUniformLocation(shdr.program,shdr.uniforms[i].name.c_str());

    int cache_size=0;
    for(int i=0;i<(int)shdr.uniforms.size();++i)
    {
        shader_obj::uniform &u=shdr.uniforms[i];
        if(u.type==shader::uniform_sampler2d || u.type==shader::uniform_sampler_cube)
            continue;
        u.cache_idx=cache_size;
        cache_size+=u.array_size*(u.type==shader::uniform_mat4?16:4);

        if(u.handler<0)
            u.type=shader::uniform_not_found;
    }
    shdr.uniform_cache.resize(cache_size);

    set_shader(-1);

    return idx;
}

render_opengl::uint render_opengl::get_uniforms_count(int shader) { return (int)shaders.get(shader).uniforms.size(); }
shader::uniform render_opengl::get_uniform(int shader,int idx) { return shaders.get(shader).uniforms[idx]; }

void render_opengl::remove_shader(int shader)
{
    if(applied_state.shader==shader)
    {
        glUseProgram(0);
        applied_state.shader=-1;
    }
    shaders.remove(shader);
}

//ToDo: uniform buffers

int render_opengl::create_uniform_buffer(int shader) { return shader; }

void render_opengl::set_uniform(int shader,int idx,const float *buf,uint count)
{
    shader_obj &s=shaders.get(shader);

    float *cache=&s.uniform_cache[s.uniforms[idx].cache_idx];
    if(memcmp(cache,buf,count*sizeof(float))==0)
        return;
    memcpy(cache,buf,count*sizeof(float));

    set_shader(shader);

    const int handler=s.uniforms[idx].handler;
    switch(s.uniforms[idx].type)
    {
        case shader::uniform_mat4: glUniformMatrix4fv(handler,count/16,false,buf); break;
        case shader::uniform_vec4: glUniform4fv(handler,count/4,buf); break;
        case shader::uniform_vec3: glUniform3fv(handler,count/3,buf); break;
        case shader::uniform_vec2: glUniform2fv(handler,count/2,buf); break;
        case shader::uniform_float: glUniform1fv(handler,count,buf); break;
        default: break;
    }
}

void render_opengl::remove_uniform_buffer(int uniform_buffer) {}

namespace
{
    int active_transform_feedback=0;

    int get_gl_element_type(vbo::element_type type)
    {
        switch(type)
        {
            case vbo::triangles: return GL_TRIANGLES;;
            case vbo::triangle_strip: return GL_TRIANGLE_STRIP;
            case vbo::points: return GL_POINTS;
            case vbo::lines: return GL_LINES;
            case vbo::line_strip: return GL_LINE_STRIP;
            default: return -1;
        }

        return -1;
    }

    int get_gl_element_type(vbo::vertex_atrib_type type)
    {
        switch(type)
        {
            case vbo::float16: return GL_HALF_FLOAT;
            case vbo::float32: return GL_FLOAT;
            case vbo::uint8: return GL_UNSIGNED_BYTE;
        }

        return GL_FLOAT;
    }

    int gl_usage(vbo::usage_hint usage)
    {
        switch(usage)
        {
            case vbo::static_draw: return GL_STATIC_DRAW;
            case vbo::dynamic_draw: return GL_DYNAMIC_DRAW;
            case vbo::stream_draw: return GL_STREAM_DRAW;
        }

        return GL_DYNAMIC_DRAW;
    }

    struct vert_buf
    {
        unsigned int id;

#ifdef USE_VAO
        unsigned int vertex_array_object;
        unsigned int active_vao_ibuf;
#endif
        vbo::layout layout;
        unsigned int stride;
        unsigned int count;
        vbo::usage_hint usage;

    public:
        void release()
        {
#ifdef USE_VAO
            if(vertex_array_object)
                glDeleteVertexArrays(1,&vertex_array_object);
            vertex_array_object=0;
#endif
            if(id)
                glDeleteBuffers(1,&id);
            id=0;
        }
    };
    render_objects<vert_buf> vert_bufs;

    struct ind_buf
    {
        unsigned int id;
        vbo::index_size type;
        unsigned int count;
        vbo::usage_hint usage;

    public:
        void release()
        {
            if(id)
                glDeleteBuffers(1,&id);
            id=0;
        }
    };
    render_objects<ind_buf> ind_bufs;
}

int render_opengl::create_vertex_buffer(const void *data,uint stride,uint count,vbo::usage_hint usage)
{
    init_extensions();

    const int idx=vert_bufs.add();
    vert_buf &v=vert_bufs.get(idx);
    glGenBuffers(1,&v.id);
    glBindBuffer(GL_ARRAY_BUFFER,v.id);
    glBufferData(GL_ARRAY_BUFFER,count*stride,data,gl_usage(usage));
    v.count=count;
    v.stride=stride;
#ifdef USE_VAO
    v.vertex_array_object=0;
    v.active_vao_ibuf=0;
#endif
    applied_state.vertex_buffer=-1;
    return idx;
}

void render_opengl::set_vertex_layout(int idx,vbo::layout layout)
{
    vert_buf &v=vert_bufs.get(idx);
    v.layout=layout;

#ifdef USE_VAO
    if(v.vertex_array_object>0)
    {
        glDeleteVertexArrays(1,&v.vertex_array_object);
        v.vertex_array_object=0;
    }
#endif
}

void render_opengl::update_vertex_buffer(int idx,const void *data)
{
    vert_buf &v=vert_bufs.get(idx);
    //if(applied_state.vertex_buffer!=idx)
    {
#ifdef USE_VAO
        glBindVertexArray(0);
#endif
        glBindBuffer(GL_ARRAY_BUFFER,v.id);
        applied_state.vertex_buffer=-1;
    }
    glBufferData(GL_ARRAY_BUFFER,v.count*v.stride,0,gl_usage(v.usage)); //orphaning
    glBufferSubData(GL_ARRAY_BUFFER,0,v.count*v.stride,data);

#ifdef USE_VAO
    //if(applied_state.vertex_buffer!=idx)
        glBindBuffer(GL_ARRAY_BUFFER,0);
#endif
}

bool render_opengl::get_vertex_data(int idx,void *data)
{
    const vert_buf &v=vert_bufs.get(idx);
    //if(applied_state.vertex_buffer!=idx)
    {
#ifdef USE_VAO
        glBindVertexArray(0);
#endif
        glBindBuffer(GL_ARRAY_BUFFER,v.id);
        applied_state.vertex_buffer=-1;
    }

#ifdef OPENGL_ES
  #ifdef __ANDROID__
    //ToDo
    return false;
  #else
    //apple hasn't GL_READ_ONLY_OES
    const GLvoid *buf=glMapBufferOES(GL_ARRAY_BUFFER,GL_WRITE_ONLY_OES);
    if(!buf)
        return false;

    memcpy(data,buf,v.stride*v.count);
    if(!glUnmapBufferOES(GL_ARRAY_BUFFER))
        return false;
  #endif
#else
    glGetBufferSubData(GL_ARRAY_BUFFER,0,v.stride*v.count,data);
#endif

#ifdef USE_VAO
    //if(applied_state.vertex_buffer!=idx)
        glBindBuffer(GL_ARRAY_BUFFER,0);
#endif
    return true;
}

void render_opengl::remove_vertex_buffer(int idx)
{
    if(active_transform_feedback==idx)
    {
        active_transform_feedback=0;
        glBindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER,0,0);
    }

    if(applied_state.vertex_buffer==idx)
    {
#ifdef USE_VAO
        glBindVertexArray(0);
#endif
        glBindBuffer(GL_ARRAY_BUFFER,0);
        applied_state.vertex_buffer=-1;
    }
    vert_bufs.remove(idx);
}

int render_opengl::create_index_buffer(const void *data,vbo::index_size size,uint indices_count,vbo::usage_hint usage)
{
    init_extensions();

    const int idx=ind_bufs.add();
    ind_buf &i=ind_bufs.get(idx);

#ifdef USE_VAO
    glBindVertexArray(0);
    applied_state.vertex_buffer=-1;
#else
    applied_state.index_buffer=-1;
#endif

    glGenBuffers(1,&i.id);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,i.id);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,size*indices_count,data,gl_usage(usage));
    i.type=size;
    i.count=indices_count;
    i.usage=usage;

#ifdef USE_VAO
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,0);
#endif
    return idx;
}

void render_opengl::update_index_buffer(int idx,const void *data)
{
    const ind_buf &i=ind_bufs.get(idx);

#ifdef USE_VAO
    glBindVertexArray(0);
    applied_state.vertex_buffer=-1;
#else
    applied_state.index_buffer=-1;
#endif

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,i.id);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,i.count*i.type,data,gl_usage(i.usage));

#ifdef USE_VAO
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,0);
#endif
}

bool render_opengl::get_index_data(int idx,void *data)
{
    const ind_buf &i=ind_bufs.get(idx);

#ifdef USE_VAO
    glBindVertexArray(0);
    applied_state.vertex_buffer=-1;
#else
    applied_state.index_buffer=-1;
#endif

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,i.id);

#ifdef OPENGL_ES
  #ifdef __ANDROID__
    //ToDo
    return false;
  #else
    //no GL_READ_ONLY_OES on apple
    const GLvoid *buf=glMapBufferOES(GL_ELEMENT_ARRAY_BUFFER,GL_WRITE_ONLY_OES);
    if(!buf)
        return false;

    memcpy(data,buf,i.type*i.count);
    if(!glUnmapBufferOES(GL_ELEMENT_ARRAY_BUFFER))
        return false;
  #endif
#else
    glGetBufferSubData(GL_ELEMENT_ARRAY_BUFFER,0,i.type*i.count,data);
#endif

#ifdef USE_VAO
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,0);
#endif
    return true;
}

void render_opengl::remove_index_buffer(int idx)
{
    if(applied_state.index_buffer==idx)
    {
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,0);
        applied_state.index_buffer=-1;
    }
    ind_bufs.remove(idx);
}

namespace
{
    const unsigned int cube_faces[]={GL_TEXTURE_CUBE_MAP_POSITIVE_X,GL_TEXTURE_CUBE_MAP_NEGATIVE_X,
                                     GL_TEXTURE_CUBE_MAP_POSITIVE_Y,GL_TEXTURE_CUBE_MAP_NEGATIVE_Y,
                                     GL_TEXTURE_CUBE_MAP_POSITIVE_Z,GL_TEXTURE_CUBE_MAP_NEGATIVE_Z};

    void gl_setup_filtration(int target,bool has_mips,texture::filter minif,texture::filter magnif,texture::filter mip)
    {
        glTexParameteri(target,GL_TEXTURE_MAG_FILTER,magnif==texture::filter_nearest?GL_NEAREST:GL_LINEAR);

        GLint filter;
        if(has_mips)
        {
            if(minif==texture::filter_nearest)
            {
                if(mip==texture::filter_nearest)
                    filter=GL_NEAREST_MIPMAP_NEAREST;
                else
                    filter=GL_NEAREST_MIPMAP_LINEAR;
            }
            else
            {
                if(mip==texture::filter_nearest)
                    filter=GL_LINEAR_MIPMAP_NEAREST;
                else
                    filter=GL_LINEAR_MIPMAP_LINEAR;
            }
        }
        else if(minif==texture::filter_nearest)
            filter=GL_NEAREST;
        else
            filter=GL_LINEAR;

        glTexParameteri(target,GL_TEXTURE_MIN_FILTER,filter);
    }

    void gl_generate_mips_pre(int gl_type)
    {
#if defined GL_GENERATE_MIPMAP && !defined OPENGL3
        glTexParameteri(gl_type,GL_GENERATE_MIPMAP,GL_TRUE);
#endif
    }

    void gl_generate_mips_post(int gl_type)
    {
#if defined GL_GENERATE_MIPMAP && !defined OPENGL3
        glTexParameteri(gl_type,GL_GENERATE_MIPMAP,GL_FALSE);
#else
        glGenerateMipmap(gl_type);
#endif
    }

    bool gl_get_format(texture::color_format format,unsigned int &source_format,unsigned int &gl_format,unsigned int &precision)
    {
        precision=GL_UNSIGNED_BYTE;
        switch(format)
        {
            case texture::color_rgb: source_format=gl_format=GL_RGB; break; //in es stored internally as rgba
            case texture::color_rgba: source_format=gl_format=GL_RGBA; break;
#ifdef USE_BGRA
            case texture::color_bgra: source_format=GL_RGBA; gl_format=GL_BGRA; break;
#endif
            case texture::greyscale: source_format=gl_format=GL_LUMINANCE; break;
#ifdef OPENGL_ES
            case texture::color_r32f: source_format=GL_RED_EXT; gl_format=GL_RED_EXT; precision=GL_FLOAT; break;
            case texture::color_rgb32f: source_format=GL_RGB; gl_format=GL_RGB; precision=GL_FLOAT; break;
            case texture::color_rgba32f: source_format=GL_RGBA; gl_format=GL_RGBA; precision=GL_FLOAT; break;

            case texture::depth16: source_format=gl_format=GL_DEPTH_COMPONENT; precision=GL_UNSIGNED_SHORT; break;
            case texture::depth32: source_format=gl_format=GL_DEPTH_COMPONENT; precision=GL_UNSIGNED_INT; break;

            case texture::etc1: source_format=gl_format=GL_ETC1_RGB8_OES; break;
            case texture::etc2: source_format=gl_format=GL_COMPRESSED_RGB8_ETC2; break;
            case texture::etc2_a1: source_format=gl_format=GL_COMPRESSED_RGB8_PUNCHTHROUGH_ALPHA1_ETC2; break;
            case texture::etc2_eac: source_format=gl_format=GL_COMPRESSED_RGBA8_ETC2_EAC; break;

            case texture::pvr_rgb2b: source_format=gl_format=GL_COMPRESSED_RGB_PVRTC_2BPPV1_IMG; break;
            case texture::pvr_rgb4b: source_format=gl_format=GL_COMPRESSED_RGB_PVRTC_4BPPV1_IMG; break;
            case texture::pvr_rgba2b: source_format=gl_format=GL_COMPRESSED_RGBA_PVRTC_2BPPV1_IMG; break;
            case texture::pvr_rgba4b: source_format=gl_format=GL_COMPRESSED_RGBA_PVRTC_4BPPV1_IMG; break;
#else
    #ifdef OPENGL3
            case texture::color_r32f: source_format=GL_R32F; gl_format=GL_RED; precision=GL_FLOAT; break;
            case texture::color_rgb32f: source_format=GL_RGB32F; gl_format=GL_RGB; precision=GL_FLOAT; break;
            case texture::color_rgba32f: source_format=GL_RGBA32F; gl_format=GL_RGBA; precision=GL_FLOAT; break;
    #else
            case texture::color_r32f: source_format=GL_R32F; gl_format=GL_RED; precision=GL_FLOAT; break;
            case texture::color_rgb32f: source_format=GL_RGB32F_ARB; gl_format=GL_RGB; precision=GL_FLOAT; break;
            case texture::color_rgba32f: source_format=GL_RGBA32F_ARB; gl_format=GL_RGBA; precision=GL_FLOAT; break;
    #endif
            case texture::depth16: source_format=GL_DEPTH_COMPONENT16; gl_format=GL_DEPTH_COMPONENT; break;
            case texture::depth24: source_format=GL_DEPTH_COMPONENT24; gl_format=GL_DEPTH_COMPONENT; break;
            case texture::depth32: source_format=GL_DEPTH_COMPONENT32; gl_format=GL_DEPTH_COMPONENT; break;
#endif
            case texture::dxt1: source_format=gl_format=GL_COMPRESSED_RGBA_S3TC_DXT1_EXT; break;
            case texture::dxt3: source_format=gl_format=GL_COMPRESSED_RGBA_S3TC_DXT3_EXT; break;
            case texture::dxt5: source_format=gl_format=GL_COMPRESSED_RGBA_S3TC_DXT5_EXT; break;

            default: return false;
        };

        return true;
    }

    struct tex_obj
    {
        unsigned int tex_id,gl_type,gl_format,gl_precision;
        bool has_mip;

    public:
        void release()
        {
            if(tex_id)
                glDeleteTextures(1,&tex_id);
            tex_id=0;
        }
    };
    render_objects<tex_obj> textures;

    void set_texture(int idx,int layer)
    {
        gl_select_multitex_layer(layer);

        const int prev_tex=applied_state.textures[layer];
        applied_state.textures[layer]=idx;
        if(idx<0)
        {
            if(prev_tex>=0)
                glBindTexture(textures.get(prev_tex).gl_type,0);
            return;
        }

        const tex_obj &t=textures.get(idx);
        glBindTexture(t.gl_type,t.tex_id);
    }

    int create_texture_(const void *data_a[6],bool is_cubemap,unsigned int width,unsigned int height,texture::color_format &format,int mip_count)
    {
        const int idx=textures.add();
        tex_obj &t=textures.get(idx);
        glGenTextures(1,&t.tex_id);
        t.gl_type=is_cubemap?GL_TEXTURE_CUBE_MAP:GL_TEXTURE_2D;
        set_texture(idx,0);

        const unsigned int source_bpp=texture::get_format_bpp(format);
#ifdef MANUAL_MIPMAP_GENERATION
        const bool bad_alignment=(source_bpp/8)%4!=0;
#else
        const bool bad_alignment=(width*source_bpp/8)%4!=0;
#endif
        if(bad_alignment)
            glPixelStorei(GL_UNPACK_ALIGNMENT,1);

        t.has_mip=(mip_count>1 || mip_count<0);

        if(t.gl_type==GL_TEXTURE_CUBE_MAP)
        {
            glTexParameteri(GL_TEXTURE_CUBE_MAP,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_CUBE_MAP,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);
#ifndef OPENGL_ES
            glTexParameteri(GL_TEXTURE_CUBE_MAP,GL_TEXTURE_WRAP_R,GL_CLAMP_TO_EDGE);
#endif
        }

        const bool is_pvrtc=format==texture::pvr_rgb2b || format==texture::pvr_rgba2b || format==texture::pvr_rgb4b || format==texture::pvr_rgba4b;
        if(is_pvrtc)
            gl_setup_filtration(t.gl_type,t.has_mip,texture::filter_linear,texture::filter_linear,texture::filter_nearest);

#ifdef OPENGL3
        if(format==texture::greyscale)
        {
            const int swizzle[]={GL_RED,GL_RED,GL_RED,GL_ONE};
            glTexParameteriv(t.gl_type,GL_TEXTURE_SWIZZLE_RGBA,swizzle);
        }
#endif

#ifndef OPENGL_ES
        if(mip_count>1) //is_platform_restrictions_ignored() &&
            glTexParameteri(t.gl_type,GL_TEXTURE_MAX_LEVEL,mip_count-1);
#endif

#ifndef MANUAL_MIPMAP_GENERATION
        if(t.has_mip && mip_count<0)
            gl_generate_mips_pre(t.gl_type);
#endif
        unsigned int source_format=0;
        gl_get_format(format,source_format,t.gl_format,t.gl_precision);

        for(int j=0;j<(is_cubemap?6:1);++j)
        {
            const unsigned char *data_pointer=data_a?(const unsigned char*)data_a[j]:0;
            unsigned int w=width,h=height;
            const int gl_type=is_cubemap?cube_faces[j]:GL_TEXTURE_2D;

#ifdef MANUAL_MIPMAP_GENERATION
            if(mip_count<0 && data_pointer)
            {
                nya_memory::tmp_buffer_scoped buf(w*h*(source_bpp/8)/2);
                for(int i=0;;++i,w=w>1?w/2:1,h=h>1?h/2:1)
                {
                    glTexImage2D(gl_type,i,source_format,w,h,0,t.gl_format,t.gl_precision,data_pointer);
                    if(w==1 && h==1)
                        break;

                    bitmap_downsample2x(data_pointer,w,h,source_bpp/8,(unsigned char *)buf.get_data());
                    data_pointer=(unsigned char *)buf.get_data();
                }
            }
            else
#endif
            {
                for(int i=0;i<(mip_count<=0?1:mip_count);++i,w=w>1?w/2:1,h=h>1?h/2:1)
                {
                    unsigned int size=0;
                    if(format<texture::dxt1)
                    {
                        size=w*h*(source_bpp/8);
                        glTexImage2D(gl_type,i,source_format,w,h,0,t.gl_format,t.gl_precision,data_pointer);
                    }
                    else
                    {
                        if(is_pvrtc)
                        {
                            if(format==texture::pvr_rgb2b || format==texture::pvr_rgba2b)
                                size=((w>16?w:16)*(h>8?h:8)*2 + 7)/8;
                            else
                                size=((w>8?w:8)*(h>8?h:8)*4 + 7)/8;

                            if(size<32)
                                break;
                        }
                        else
                            size=(w>4?w:4)/4 * (h>4?h:4)/4 * source_bpp*2;

                        glCompressedTexImage2D(gl_type,i,t.gl_format,w,h,0,size,data_pointer);
                    }
                    data_pointer+=size;
                }
            }
        }

        if(bad_alignment)
            glPixelStorei(GL_UNPACK_ALIGNMENT,4);

#ifndef MANUAL_MIPMAP_GENERATION
        if(t.has_mip && mip_count<0)
            gl_generate_mips_post(t.gl_type);
#endif

#ifdef OPENGL_ES
        if(t.gl_format==GL_RGB)
            t.gl_format=GL_RGBA;
#endif
        return idx;
    }
}

int render_opengl::create_texture(const void *data,uint width,uint height,texture::color_format &format,int mip_count)
{
    init_extensions();

    const void *data_a[6]={data};
    return create_texture_(data_a,false,width,height,format,mip_count);
}

int render_opengl::create_cubemap(const void *data[6],uint width,texture::color_format &format,int mip_count)
{
    init_extensions();

    return create_texture_(data,true,width,width,format,mip_count);
}

void render_opengl::update_texture(int idx,const void *data,uint x,uint y,uint width,uint height,int mip)
{
    const tex_obj &t=textures.get(idx);
    set_texture(idx,0);

#ifdef __ANDROID__ //adreno 4xx fix
    glFinish();
#endif

    if(t.has_mip && mip<0)
        gl_generate_mips_pre(t.gl_type);

    glTexSubImage2D(t.gl_type,mip<0?0:mip,x,y,width,height,t.gl_format,t.gl_precision,data);

    if(t.has_mip && mip<0)
    {
        gl_generate_mips_post(t.gl_type);

    #if defined __APPLE__ && defined OPENGL_ES //ios devices don't wait for mip generation
        glFinish();
    #endif
    }
}

void render_opengl::set_texture_wrap(int idx,texture::wrap s,texture::wrap t)
{
    const tex_obj &tex=textures.get(idx);
    set_texture(idx,0);

    const texture::wrap wraps[]={s,t};
    const GLint pnames[]={GL_TEXTURE_WRAP_S,GL_TEXTURE_WRAP_T};
    for(int i=0;i<2;++i)
    {
        switch(wraps[i])
        {
            case texture::wrap_clamp:glTexParameteri(tex.gl_type,pnames[i],GL_CLAMP_TO_EDGE);break;
            case texture::wrap_repeat:glTexParameteri(tex.gl_type,pnames[i],GL_REPEAT);break;
            case texture::wrap_repeat_mirror:glTexParameteri(tex.gl_type,pnames[i],GL_MIRRORED_REPEAT);break;
        }
    }
}

void render_opengl::set_texture_filter(int idx,texture::filter minification,texture::filter magnification,texture::filter mipmap,uint aniso)
{
    static int max_aniso= -1;
    if(max_aniso<0)
    {
        GLfloat f=0.0f;
        glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &f);
        max_aniso=int(f);
    }

    if((int)aniso>max_aniso)
        aniso=(uint)max_aniso;
    if(!aniso)
        aniso=1;

    const tex_obj &t=textures.get(idx);
    set_texture(idx,0);
    glTexParameterf(t.gl_type,GL_TEXTURE_MAX_ANISOTROPY_EXT,float(aniso));
    gl_setup_filtration(t.gl_type,t.has_mip,minification,magnification,mipmap);
}

bool render_opengl::get_texture_data(int texture,uint x,uint y,uint w,uint h,void *data)
{
    const tex_obj &t=textures.get(texture);

    //compressed formats are not supported
    switch(t.gl_format)
    {
#ifdef OPENGL_ES
        case GL_ETC1_RGB8_OES:
        case GL_COMPRESSED_RGB8_PUNCHTHROUGH_ALPHA1_ETC2:
        case GL_COMPRESSED_RGBA8_ETC2_EAC:
        case GL_COMPRESSED_RGB_PVRTC_2BPPV1_IMG:
        case GL_COMPRESSED_RGB_PVRTC_4BPPV1_IMG:
        case GL_COMPRESSED_RGBA_PVRTC_2BPPV1_IMG:
        case GL_COMPRESSED_RGBA_PVRTC_4BPPV1_IMG:
#endif
        case GL_COMPRESSED_RGBA_S3TC_DXT1_EXT:
        case GL_COMPRESSED_RGBA_S3TC_DXT3_EXT:
        case GL_COMPRESSED_RGBA_S3TC_DXT5_EXT:
            return false;
    }

#ifdef __ANDROID__ //adreno 4xx fix
    glFinish();
#endif

    glViewport(0,0,x+w,y+h);

    uint tmp_fbo;
    glGenFramebuffers(1,&tmp_fbo);
    glBindFramebuffer(GL_FRAMEBUFFER,tmp_fbo);

    if(t.gl_type==GL_TEXTURE_CUBE_MAP)
    {
        unsigned int size=(w-x)*(h-y);
        if(t.gl_precision==GL_FLOAT)
            size*=4;

        switch(t.gl_format)
        {
            case GL_RGB: size*=3; break;

            case GL_RGBA: 
#ifdef USE_BGRA
			case GL_BGRA:
#endif
			case GL_COMPRESSED_RGBA_S3TC_DXT1_EXT:
            case GL_COMPRESSED_RGBA_S3TC_DXT3_EXT:
            case GL_COMPRESSED_RGBA_S3TC_DXT5_EXT:
                size *=4;
                break;
        }

        for(int i=0;i<6;++i)
        {
            glFramebufferTexture2D(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT0,gl_cube_type(i),t.tex_id,0);
            glReadPixels(x,y,w,h,t.gl_format,t.gl_precision,(char *)data+size*i);
        }
    }
    else
    {
        glFramebufferTexture2D(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT0,t.gl_type,t.tex_id,0);
        glReadPixels(x,y,w,h,t.gl_format,t.gl_precision,data);
    }

    glBindFramebuffer(GL_FRAMEBUFFER,default_fbo_idx);
    glDeleteFramebuffers(1,&tmp_fbo);

    applied_state.target= -1;
    glViewport(applied_state.viewport.x,applied_state.viewport.y,applied_state.viewport.width,applied_state.viewport.height);
    return true;
}

void render_opengl::remove_texture(int idx)
{
    for(int i=0;i<state::max_layers;++i)
    {
        if(applied_state.textures[i]!=idx)
            continue;

        applied_state.textures[i]=-1;
        gl_select_multitex_layer(i);
        const tex_obj &t=textures.get(idx);
        glBindTexture(t.gl_type,0);
    }
    textures.remove(idx);
}

unsigned int render_opengl::get_max_texture_dimention()
{
    static unsigned int max_tex_size=0;
    if(!max_tex_size)
    {
        GLint texSize;
        glGetIntegerv(GL_MAX_TEXTURE_SIZE, &texSize);
        max_tex_size=texSize;
    }

    return max_tex_size;
}

bool render_opengl::is_texture_format_supported(texture::color_format format)
{
    unsigned int source_format,gl_format,precision;
    return gl_get_format(format,source_format,gl_format,precision);
}

//target

namespace
{
    struct ms_buffer
    {
        unsigned int fbo,buf;
        unsigned int width,height,samples,format;

        void create(unsigned int w,unsigned int h,unsigned int f,unsigned int s)
        {
            if(!glRenderbufferStorageMultisample)
                return;

            if(!w || !h || s<1)
            {
                release();
                return;
            }

            if(w==width && h==height && s==samples && f==format)
                return;

#ifdef OPENGL_ES
            if(f==GL_RGBA)
                f=GL_RGB5_A1;
#endif
            glGenRenderbuffers(1,&buf);
            glBindRenderbuffer(GL_RENDERBUFFER,buf);
            glRenderbufferStorageMultisample(GL_RENDERBUFFER,s,f,w,h);
            glBindRenderbuffer(GL_RENDERBUFFER,0);
            glGenFramebuffers(1,&fbo);

            width=w,height=h,samples=s,format=f;
        }

        void resolve(int from,tex_obj &tex,int cubemap_side,int attachment_idx) const;

        void release()
        {
            if(buf) glDeleteRenderbuffers(1,&buf);
            if(fbo) glDeleteFramebuffers(1,&fbo);
            *this=ms_buffer();
        }

        ms_buffer(): buf(0),width(0),height(0),samples(0),fbo(0) {}
    };

    struct fbo_obj
    {
        struct attachment
        {
            int tex_idx,cubemap_side;
            ms_buffer multisample;

            attachment(): tex_idx(-1),cubemap_side(-1) {}
        };

        std::vector<attachment> color_attachments;
        unsigned int id;
        int depth_tex_idx;
        ms_buffer multisample_depth;
        bool depth_only;

        void release()
        {
            for(size_t i=0;i<color_attachments.size();++i)
                color_attachments[i].multisample.release();
            multisample_depth.release();
            if(id)
                glDeleteFramebuffers(1,&id);
            *this=fbo_obj();
        }
    };
    render_objects<fbo_obj> fbos;

    void ms_buffer::resolve(int from,tex_obj &tex,int cubemap_side,int attachment_idx) const
    {
        if(!buf)
            return;

        const int gl_type=cubemap_side<0?tex.gl_type:gl_cube_type(cubemap_side);
        glBindFramebuffer(GL_FRAMEBUFFER,fbo);
        glFramebufferTexture2D(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT0,gl_type,tex.tex_id,0);
        glBindFramebuffer(GL_FRAMEBUFFER,default_fbo_idx);

#if defined OPENGL_ES && defined __APPLE__
        if(attachment_idx!=0) //ToDo
            return;

        glBindFramebuffer(GL_READ_FRAMEBUFFER_APPLE,from);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER_APPLE,fbo);
        glResolveMultisampleFramebufferAPPLE();
        glBindFramebuffer(GL_READ_FRAMEBUFFER_APPLE,default_fbo_idx);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER_APPLE,default_fbo_idx);
#else
    #ifdef OPENGL_ES
        if(glBindFramebuffer)
    #endif
        {
            glBindFramebuffer(GL_READ_FRAMEBUFFER,from);
            glBindFramebuffer(GL_DRAW_FRAMEBUFFER,fbo);
            glReadBuffer(GL_COLOR_ATTACHMENT0+attachment_idx);
            glBlitFramebuffer(0,0,width,height,0,0,width,height,GL_COLOR_BUFFER_BIT,GL_NEAREST);
            glReadBuffer(GL_COLOR_ATTACHMENT0);
            glBindFramebuffer(GL_READ_FRAMEBUFFER,default_fbo_idx);
            glBindFramebuffer(GL_DRAW_FRAMEBUFFER,default_fbo_idx);
        }

        applied_state.target= -1;
#endif
    }
}

int render_opengl::create_target(uint width,uint height,uint samples,const int *attachment_textures,
                                 const int *attachment_sides,uint attachment_count,int depth_texture)
{
    init_extensions();

    if(default_fbo_idx<0)
        glGetIntegerv(GL_FRAMEBUFFER_BINDING,&default_fbo_idx);

    int idx=fbos.add();
    fbo_obj &f=fbos.get(idx);

    glGenFramebuffers(1,&f.id);
    glBindFramebuffer(GL_FRAMEBUFFER,f.id);

    bool has_color=false;
    f.color_attachments.resize(attachment_count);
    for(uint i=0;i<attachment_count;++i)
    {
        if(attachment_textures[i]<0)
            continue;

        f.color_attachments[i].tex_idx=attachment_textures[i];
        f.color_attachments[i].cubemap_side=attachment_sides[i];
        has_color=true;

        const tex_obj &t=textures.get(attachment_textures[i]);
        if(samples>1)
        {
            f.color_attachments[i].multisample.create(width,height,t.gl_format,samples);
            glFramebufferRenderbuffer(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT0+i,GL_RENDERBUFFER,f.color_attachments[i].multisample.buf);
        }
        else
        {
            const int gl_type=attachment_sides[i]<0?t.gl_type:gl_cube_type(attachment_sides[i]);
            glFramebufferTexture2D(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT0+i,gl_type,t.tex_id,0);
        }
    }

    if(depth_texture>=0)
    {
        const tex_obj &t=textures.get(depth_texture);
        if(samples>1)
        {
            f.multisample_depth.create(width,height,t.gl_format,samples);
            glFramebufferRenderbuffer(GL_FRAMEBUFFER,GL_DEPTH_ATTACHMENT,GL_RENDERBUFFER,f.multisample_depth.buf);
        }
        else
            glFramebufferTexture2D(GL_FRAMEBUFFER,GL_DEPTH_ATTACHMENT,GL_TEXTURE_2D,t.tex_id,0);
        f.depth_tex_idx=depth_texture;
    }
    else
        f.depth_tex_idx=-1;

    f.depth_only=!has_color && depth_texture>=0;

    glBindFramebuffer(GL_FRAMEBUFFER,applied_state.target>=0?fbos.get(applied_state.target).id:default_fbo_idx);
    return idx;
}

void render_opengl::resolve_target(int idx)
{
    const fbo_obj &f=fbos.get(idx);
    for(int i=0;i<(int)f.color_attachments.size();++i)
    {
        const fbo_obj::attachment &a=f.color_attachments[i];
        if(a.tex_idx<0)
            continue;

        tex_obj &tex=textures.get(a.tex_idx);
        a.multisample.resolve(f.id,tex,a.cubemap_side,int(i));

        if(tex.has_mip)
        {
            gl_select_multitex_layer(0);
            glBindTexture(tex.gl_type, tex.tex_id);
            applied_state.textures[0]=tex.tex_id;
            glGenerateMipmap(tex.gl_type);
        }
    }
}

void render_opengl::remove_target(int idx)
{
    if(applied_state.target==idx)
    {
        glBindFramebuffer(GL_FRAMEBUFFER,default_fbo_idx);
        applied_state.target=-1;
    }
    return fbos.remove(idx);
}

unsigned int render_opengl::get_max_target_attachments()
{
    static int max_attachments= -1;
    if(max_attachments<0)
        glGetIntegerv(GL_MAX_COLOR_ATTACHMENTS, &max_attachments);
    if(max_attachments<0)
        max_attachments=1;
    return max_attachments;
}

unsigned int render_opengl::get_max_target_msaa()
{
    static int max_ms=-1;

#if defined OPENGL_ES && !defined __APPLE__
    init_extensions();

    if (!glBlitFramebuffer || !glReadBuffer || !glRenderbufferStorageMultisample)
        return 1;
#endif

    if(max_ms<0)
        glGetIntegerv(GL_MAX_SAMPLES,&max_ms);

    return max_ms>1?max_ms:1;
}

static void apply_viewport_state(render_api_interface::viewport_state s)
{
    if(ignore_cache_vp)
    {
        if(default_fbo_idx<0)
            glGetIntegerv(GL_FRAMEBUFFER_BINDING,&default_fbo_idx);
        init_extensions();
    }

    if(s.target!=applied_state.target || ignore_cache_vp)
    {
        glBindFramebuffer(GL_FRAMEBUFFER,s.target>=0?fbos.get(s.target).id:default_fbo_idx);

#ifndef OPENGL_ES
        const bool no_color=s.target>=0 && fbos.get(s.target).depth_only;
        if(s.target>=0 && (no_color!=was_fbo_without_color || ignore_cache_vp))
        {
            const int buffer=no_color?GL_NONE:GL_COLOR_ATTACHMENT0;
            glDrawBuffer(buffer);
            glReadBuffer(buffer);
            was_fbo_without_color=no_color;
        }
#endif
    }

    if(applied_state.viewport!=s.viewport || ignore_cache_vp)
        glViewport(s.viewport.x,s.viewport.y,s.viewport.width,s.viewport.height);

    if(memcmp(applied_state.clear_color,s.clear_color,sizeof(applied_state.clear_color))!=0 || ignore_cache_vp)
        glClearColor(s.clear_color[0],s.clear_color[1],s.clear_color[2],s.clear_color[3]);

    if(applied_state.clear_depth!=s.clear_depth || ignore_cache_vp)
    {
#ifdef OPENGL_ES
        glClearDepthf(s.clear_depth);
#else
        glClearDepth(s.clear_depth);
#endif
    }

    if(s.scissor_enabled!=applied_state.scissor_enabled || ignore_cache_vp)
    {
        if(s.scissor_enabled)
            glEnable(GL_SCISSOR_TEST);
        else
            glDisable(GL_SCISSOR_TEST);
    }

    if(s.scissor!=applied_state.scissor)
        glScissor(s.scissor.x,s.scissor.y,s.scissor.width,s.scissor.height);

    *(render_api_interface::viewport_state*)&applied_state=s;
	ignore_cache_vp=false;
}

inline GLenum gl_blend_mode(blend::mode m)
{
    switch(m)
    {
        case blend::zero: return GL_ZERO;
        case blend::one: return GL_ONE;
        case blend::src_color: return GL_SRC_COLOR;
        case blend::inv_src_color: return GL_ONE_MINUS_SRC_COLOR;
        case blend::src_alpha: return GL_SRC_ALPHA;
        case blend::inv_src_alpha: return GL_ONE_MINUS_SRC_ALPHA;
        case blend::dst_color: return GL_DST_COLOR;
        case blend::inv_dst_color: return GL_ONE_MINUS_DST_COLOR;
        case blend::dst_alpha: return GL_DST_ALPHA;
        case blend::inv_dst_alpha: return GL_ONE_MINUS_DST_ALPHA;
    }

    return GL_ONE;
}

void render_opengl::clear(const viewport_state &s,bool color,bool depth,bool stencil)
{
    apply_viewport_state(s);

    GLbitfield mode=0;
    if(color)
    {
        mode|=GL_COLOR_BUFFER_BIT;
        if(!applied_state.color_write)
        {
            glColorMask(true,true,true,true);
            applied_state.color_write=true;
        }
    }

    if(depth)
    {
        mode|=GL_DEPTH_BUFFER_BIT;
        if(!applied_state.zwrite)
        {
            glDepthMask(true);
            applied_state.zwrite=true;
        }
    }

    if(stencil)
        mode|=GL_STENCIL_BUFFER_BIT;

    glClear(mode);
}

void render_opengl::invalidate_cached_state()
{
    ignore_cache = true;
	ignore_cache_vp = true;

    applied_state.index_buffer=applied_state.vertex_buffer= -1;
    applied_state.shader=applied_state.uniform_buffer= -1;
    active_layer=-1;
    for(int i=0;i<state::max_layers;++i)
    {
        applied_state.textures[i]=-1;
        gl_select_multitex_layer(i);
        glBindTexture(GL_TEXTURE_2D,0);
        glBindTexture(GL_TEXTURE_CUBE_MAP,0);
    }

#ifndef USE_VAO
    applied_layout=vbo::layout();
    //ToDo: disable layout client states
#endif
}

namespace { nya_math::mat4 modelview, projection; }

void render_opengl::set_camera(const nya_math::mat4 &mv,const nya_math::mat4 &p)
{
    modelview=mv;
    projection=p;
}

void render_opengl::apply_state(const state &c)
{
    state &a=applied_state;

    apply_viewport_state(c);

    if(c.blend!=a.blend || ignore_cache)
    {
        if(c.blend)
            glEnable(GL_BLEND);
        else
            glDisable(GL_BLEND);
        a.blend=c.blend;
    }

    if(c.blend_src!=a.blend_src || c.blend_dst!=a.blend_dst || ignore_cache)
    {
        glBlendFuncSeparate(gl_blend_mode(c.blend_src),gl_blend_mode(c.blend_dst),GL_ONE,GL_ONE);
        a.blend_src=c.blend_src,a.blend_dst=c.blend_dst;
    }

    if(c.cull_face!=a.cull_face || ignore_cache)
    {
        if(c.cull_face)
            glEnable(GL_CULL_FACE);
        else
            glDisable(GL_CULL_FACE);
        a.cull_face=c.cull_face;
    }

    if(c.cull_order!=a.cull_order || ignore_cache)
    {
        if(c.cull_order==cull_face::cw)
            glFrontFace(GL_CW);
        else
            glFrontFace(GL_CCW);
        a.cull_order=c.cull_order;
    }

    if(c.depth_test!=a.depth_test || ignore_cache)
    {
        if(c.depth_test)
            glEnable(GL_DEPTH_TEST);
        else
            glDisable(GL_DEPTH_TEST);
        a.depth_test=c.depth_test;
    }

    if(c.depth_comparsion!=a.depth_comparsion || ignore_cache)
    {
        switch(c.depth_comparsion)
        {
            case depth_test::never: glDepthFunc(GL_NEVER); break;
            case depth_test::less: glDepthFunc(GL_LESS); break;
            case depth_test::equal: glDepthFunc(GL_EQUAL); break;
            case depth_test::greater: glDepthFunc(GL_GREATER); break;
            case depth_test::not_less: glDepthFunc(GL_GEQUAL); break;
            case depth_test::not_equal: glDepthFunc(GL_NOTEQUAL); break;
            case depth_test::not_greater: glDepthFunc(GL_LEQUAL); break;
            case depth_test::allways: glDepthFunc(GL_ALWAYS); break;
        }
        a.depth_comparsion=c.depth_comparsion;
    }

    if(c.zwrite!=a.zwrite || ignore_cache)
    {
        glDepthMask(c.zwrite);
        a.zwrite=c.zwrite;
    }

    if(c.color_write!=a.color_write || ignore_cache)
    {
        glColorMask(c.color_write,c.color_write,c.color_write,c.color_write);
        a.color_write=c.color_write;
    }

    for(int i=0;i<c.max_layers;++i)
    {
        if(c.textures[i]!=a.textures[i])
            set_texture(c.textures[i],i);
    }

    ignore_cache = false;
}

template<bool transform_feedback>void draw_(const nya_render::render_api_interface::state &s)
{
    if(s.vertex_buffer<0 || s.shader<0)
        return;

    set_shader(s.shader);

    shader_obj &shdr=shaders.get(s.shader);
    if(shdr.mat_mvp>=0)
    {
        const nya_math::mat4 mvp=modelview*projection;
        glUniformMatrix4fv(shdr.mat_mvp,1,false,mvp[0]);
    }
    if(shdr.mat_mv>=0)
        glUniformMatrix4fv(shdr.mat_mv,1,false,modelview[0]);
    if(shdr.mat_p>=0)
        glUniformMatrix4fv(shdr.mat_p,1,false,projection[0]);

    vert_buf &v=vert_bufs.get(s.vertex_buffer);

    if(s.vertex_buffer!=applied_state.vertex_buffer)
    {
#ifdef USE_VAO
        if(v.vertex_array_object>0)
        {
            glBindVertexArray(v.vertex_array_object);
        }
        else
        {
            glGenVertexArrays(1,&v.vertex_array_object);
            glBindVertexArray(v.vertex_array_object);

            applied_layout=vbo::layout();
            v.active_vao_ibuf=0;
#endif
            glBindBuffer(GL_ARRAY_BUFFER,v.id);

            glEnableVertexAttribArray(vertex_attribute);
            glVertexAttribPointer(vertex_attribute,v.layout.pos.dimension,get_gl_element_type(v.layout.pos.type),true,
                                  v.stride,(void*)(ptrdiff_t)(v.layout.pos.offset));
            for(unsigned int i=0;i<vbo::max_tex_coord;++i)
            {
                const vbo::layout::attribute &tc=v.layout.tc[i];
                if(tc.dimension>0)
                {
                    //if(vobj.vertex_stride!=active_attributes.vertex_stride || !tc.compare(active_attributes.tcs[i]))
                    {
                        if(!applied_layout.tc[i].dimension)
                            glEnableVertexAttribArray(tc0_attribute+i);
                        glVertexAttribPointer(tc0_attribute+i,tc.dimension,get_gl_element_type(tc.type),true,
                                              v.stride,(void*)(ptrdiff_t)(tc.offset));
                    }
                }
                else if(applied_layout.tc[i].dimension>0)
                    glDisableVertexAttribArray(tc0_attribute+i);
            }

            if(v.layout.normal.dimension>0)
            {
                //if(vobj.vertex_stride!=active_attributes.vertex_stride || !vobj.normals.compare(active_attributes.normals))
                {
                    if(!applied_layout.normal.dimension)
                        glEnableVertexAttribArray(normal_attribute);
                    glVertexAttribPointer(normal_attribute,3,get_gl_element_type(v.layout.normal.type),true,
                                          v.stride,(void*)(ptrdiff_t)(v.layout.normal.offset));
                }
            }
            else if(applied_layout.normal.dimension>0)
                glDisableVertexAttribArray(normal_attribute);

            if(v.layout.color.dimension>0)
            {
                //if(vobj.vertex_stride!=active_attributes.vertex_stride || !vobj.colors.compare(active_attributes.colors))
                {
                    if(!applied_layout.color.dimension)
                        glEnableVertexAttribArray(color_attribute);
                    glVertexAttribPointer(color_attribute,v.layout.color.dimension,get_gl_element_type(v.layout.color.type),true,
                                          v.stride,(void*)(ptrdiff_t)(v.layout.color.offset));
                }
            }
            else if(applied_layout.color.dimension>0)
                glDisableVertexAttribArray(color_attribute);
            applied_layout=v.layout;
#ifdef USE_VAO
        }
#endif
        applied_state.vertex_buffer=s.vertex_buffer;
    }

    const int gl_elem=get_gl_element_type(s.primitive);
    if(s.index_buffer>=0)
    {
        ind_buf &i=ind_bufs.get(s.index_buffer);

#ifdef USE_VAO
        if(i.id!=v.active_vao_ibuf)
        {
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,i.id);
            v.active_vao_ibuf=i.id;
        }
#else
        if(s.index_buffer!=applied_state.index_buffer)
        {
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,i.id);
            applied_state.index_buffer=s.index_buffer;
        }
#endif
        const unsigned int gl_elem_type=(i.type==vbo::index4b?GL_UNSIGNED_INT:GL_UNSIGNED_SHORT);
#ifdef USE_INSTANCING
        if(s.instances_count>1 && glDrawElementsInstancedARB)
            glDrawElementsInstancedARB(gl_elem,s.index_count,gl_elem_type,(void*)(ptrdiff_t)(s.index_offset*i.type),s.instances_count);
        else
#endif
        if(transform_feedback)
        {
            glBeginTransformFeedback(get_gl_element_type(s.primitive));
            glDrawElements(gl_elem,s.index_count,gl_elem_type,(void*)(ptrdiff_t)(s.index_offset*i.type));
            glEndTransformFeedback();
        }
        else
            glDrawElements(gl_elem,s.index_count,gl_elem_type,(void*)(ptrdiff_t)(s.index_offset*i.type));
    }
    else
    {
#ifdef USE_INSTANCING
        if(s.instances_count>1 && glDrawArraysInstancedARB)
            glDrawArraysInstancedARB(gl_elem,s.index_offset,s.index_count,s.instances_count);
        else
#endif
        if(transform_feedback)
        {
            glBeginTransformFeedback(get_gl_element_type(s.primitive));
            glDrawArrays(gl_elem,s.index_offset,s.index_count);
            glEndTransformFeedback();
        }
        else
            glDrawArrays(gl_elem,s.index_offset,s.index_count);
    }
}

void render_opengl::draw(const state &s)
{
    apply_state(s);
    draw_<false>(s);
}

void render_opengl::transform_feedback(const tf_state &s)
{
    if(!is_transform_feedback_supported())
        return;

    //glEnable(GL_RASTERIZER_DISCARD);
    const vert_buf &dst=vert_bufs.get(s.vertex_buffer_out);
    if(!s.out_offset)
        glBindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER,0,dst.id);
    else
        glBindBufferRange(GL_TRANSFORM_FEEDBACK_BUFFER,0,dst.id,s.out_offset*dst.stride,s.index_count*dst.stride);
    active_transform_feedback=s.vertex_buffer_out;

    state ss=applied_state;
    (render_state &)ss=s;
    for(int i=0;i<s.max_layers;++i)
    {
        if(s.textures[i]!=applied_state.textures[i])
            set_texture(s.textures[i],i);
    }
    draw_<true>(ss);
    //glDisable(GL_RASTERIZER_DISCARD);
}

bool render_opengl::is_transform_feedback_supported()
{
#ifdef NO_EXTENSIONS_INIT
    return true;
#else
    return glBeginTransformFeedback!=0;
#endif
}

unsigned int render_opengl::get_gl_texture_id(int idx)
{
    return textures.get(idx).tex_id;
}

void render_opengl::gl_bind_texture2d(uint gl_tex,uint layer)
{
    gl_bind_texture(GL_TEXTURE_2D,gl_tex,layer);
}

void render_opengl::gl_bind_texture(uint gl_type,uint gl_tex,uint layer)
{
    set_texture(-1,layer);
    gl_select_multitex_layer((int)layer);
    glBindTexture(gl_type, gl_tex);
}

void render_opengl::log_errors(const char *place)
{
    for(int i=glGetError();i!=GL_NO_ERROR;i=glGetError())
    {
        log()<<"gl error: ";
        switch(i)
        {
            case GL_INVALID_ENUM: log()<<"invalid enum"; break;
            case GL_INVALID_VALUE: log()<<"invalid value"; break;
            case GL_INVALID_OPERATION: log()<<"invalid operation"; break;
#if !defined OPENGL_ES && !defined OPENGL3
            case GL_STACK_OVERFLOW: log()<<"stack overflow"; break;
            case GL_STACK_UNDERFLOW: log()<<"stack underflow"; break;
#endif
            case GL_OUT_OF_MEMORY: log()<<"out of memory"; break;

            default: log()<<"unknown"; break;
        }

        log()<<" ("<<i<<")";
        if(place)
            log()<<" at "<<place<<"\n";
        else
            log()<<"\n";
    }
}

bool render_opengl::has_extension(const char *name) { return ::has_extension(name); }
void *render_opengl::get_extension(const char*name) { return ::get_extension(name); }

namespace
{
#ifdef GL_DEBUG_OUTPUT
  #ifdef _WIN32
    void CALLBACK debug_log(GLenum source,GLenum type,GLuint id,GLenum severity,GLsizei length,const GLchar *message,GLvoid *userParam)
  #else
    void debug_log(GLenum source,GLenum type,GLuint id,GLenum severity,GLsizei length,const GLchar *message,const GLvoid *userParam)
  #endif
    {
        const char* source_str;
        const char* type_str;
        const char* severity_str;

        switch (source)
        {
            case GL_DEBUG_SOURCE_API:               source_str = "api"; break;
            case GL_DEBUG_SOURCE_WINDOW_SYSTEM:     source_str = "window system"; break;
            case GL_DEBUG_SOURCE_SHADER_COMPILER:   source_str = "shader compiler"; break;
            case GL_DEBUG_SOURCE_THIRD_PARTY:       source_str = "third party"; break;
            case GL_DEBUG_SOURCE_APPLICATION:       source_str = "application"; break;
            case GL_DEBUG_SOURCE_OTHER:             source_str = "other"; break;
            default:                                source_str = "unknown"; break;
        }

        switch (type)
        {
            case GL_DEBUG_TYPE_ERROR:               type_str = "error";  break;
            case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR: type_str = "deprecated behaviour"; break;
            case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:  type_str = "undefined behaviour"; break;
            case GL_DEBUG_TYPE_PORTABILITY:         type_str = "portability"; break;
            case GL_DEBUG_TYPE_PERFORMANCE:         type_str = "performance"; break;
            case GL_DEBUG_TYPE_MARKER:              type_str = "marker"; break;
            case GL_DEBUG_TYPE_OTHER:               type_str = "other"; break;
            default:                                type_str = "unknown"; break;
        }

        switch (severity)
        {
            case GL_DEBUG_SEVERITY_HIGH:         severity_str = "high"; break;
            case GL_DEBUG_SEVERITY_MEDIUM:       severity_str = "medium"; break;
            case GL_DEBUG_SEVERITY_LOW:          severity_str = "low"; break;
            case GL_DEBUG_SEVERITY_NOTIFICATION: severity_str = "notification"; break;
            default:                             severity_str = "unknown"; break;
        }

        nya_log::log("gl log [%s] %s %s %d: %s\n", severity_str, source_str, type_str, id, message);
        if(severity!=GL_DEBUG_SEVERITY_NOTIFICATION)
            severity_str=severity_str;
    }
#endif
    bool log_set=false;
}

void render_opengl::enable_debug(bool synchronous)
{
    if(log_set)
        return;
    log_set=true;

#ifdef GL_DEBUG_OUTPUT
  #ifndef NO_EXTENSIONS_INIT
    init_extensions();
    if(!glDebugMessageCallback)
    {
        nya_log::log("unable to set opengl log\n");
        return;
    }
  #endif

    glEnable(GL_DEBUG_OUTPUT);
    if(synchronous)
        glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
    glDebugMessageCallback(debug_log,NULL);
#endif
}

render_opengl &render_opengl::get() { static render_opengl *api = new render_opengl(); return *api; }
}
