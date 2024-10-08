//nya-engine (C) nyan.developer@gmail.com released under the MIT license (see LICENSE)

#pragma once

#ifdef _WIN32
    #define NOMINMAX
    #include <windows.h>
    #include <gl/gl.h>
    #include <gl/glext.h>
#elif defined __APPLE__
    #include "TargetConditionals.h"
    #define GL_SILENCE_DEPRECATION
    #if TARGET_OS_IPHONE || TARGET_IPHONE_SIMULATOR
        #import <OpenGLES/ES2/gl.h>
        #import <OpenGLES/ES2/glext.h>
        #define OPENGL_ES
        #define NO_EXTENSIONS_INIT
    #else
        #include <OpenGL/gl3.h>
        #include <OpenGL/glext.h>
        #define OPENGL3
        #define NO_EXTENSIONS_INIT
    #endif
#elif __ANDROID__ || EMSCRIPTEN
    #include <GLES2/gl2.h>
    #define GL_GLEXT_PROTOTYPES
    #include <GLES2/gl2ext.h>
    #define OPENGL_ES
    #define NO_EXTENSIONS_INIT
#else
    #include <GL/glx.h>
    #include <GL/gl.h>
    #include <GL/glext.h>
#endif

namespace
{
#ifndef GL_VERTEX_SHADER
    #define GL_VERTEX_SHADER GL_VERTEX_SHADER_ARB
#endif

#ifndef GL_FRAGMENT_SHADER
    #define GL_FRAGMENT_SHADER GL_FRAGMENT_SHADER_ARB
#endif

#ifdef NO_EXTENSIONS_INIT
  #if !defined(OPENGL3) && !defined(OPENGL_ES)
    #define glBindBufferBase glBindBufferBaseEXT
    #define glBindBufferRange glBindBufferRangeEXT
    #define glBeginTransformFeedback glBeginTransformFeedbackEXT
    #define glEndTransformFeedback glEndTransformFeedbackEXT
    #define glTransformFeedbackVaryings glTransformFeedbackVaryingsEXT
  #endif
#endif

#ifndef GL_INTERLEAVED_ATTRIBS
    #define GL_INTERLEAVED_ATTRIBS GL_INTERLEAVED_ATTRIBS_EXT
#endif

#if defined OPENGL3 || defined __APPLE__
    #define USE_VAO
#endif

#if !defined OPENGL_ES || defined __APPLE__
    #define USE_INSTANCING
#endif

#ifdef OPENGL_ES
    #define glGenVertexArrays glGenVertexArraysOES
    #define glBindVertexArray glBindVertexArrayOES
    #define glDeleteVertexArrays glDeleteVertexArraysOES
    #ifdef __APPLE__
        #define glDrawElementsInstancedARB glDrawElementsInstancedEXT
        #define glDrawArraysInstancedARB glDrawArraysInstancedEXT
    #endif
#elif defined __APPLE__ && !defined OPENGL3
    #define glGenVertexArrays glGenVertexArraysAPPLE
    #define glBindVertexArray glBindVertexArrayAPPLE
    #define glDeleteVertexArrays glDeleteVertexArraysAPPLE
#endif

#ifdef OPENGL_ES
    #define GL_HALF_FLOAT GL_HALF_FLOAT_OES
#endif

#ifndef GL_HALF_FLOAT
    #define GL_HALF_FLOAT GL_HALF_FLOAT_ARB
#endif

#ifndef GL_ARRAY_BUFFER
    #define GL_ARRAY_BUFFER GL_ARRAY_BUFFER_ARB
#endif

#ifndef GL_DYNAMIC_DRAW
    #define GL_DYNAMIC_DRAW GL_DYNAMIC_DRAW_ARB
#endif

#ifndef GL_STATIC_DRAW
    #define GL_STATIC_DRAW GL_STATIC_DRAW_ARB
#endif

#ifndef GL_STREAM_DRAW
    #define GL_STREAM_DRAW GL_STREAM_DRAW_ARB
#endif

#ifndef GL_ELEMENT_ARRAY_BUFFER
    #define GL_ELEMENT_ARRAY_BUFFER GL_ELEMENT_ARRAY_BUFFER_ARB
#endif

#ifndef GL_TRANSFORM_FEEDBACK_BUFFER
    #define GL_TRANSFORM_FEEDBACK_BUFFER GL_TRANSFORM_FEEDBACK_BUFFER_EXT
#endif

#ifndef GL_RASTERIZER_DISCARD
    #define GL_RASTERIZER_DISCARD GL_RASTERIZER_DISCARD_EXT
#endif

#ifdef OPENGL3
    #ifdef GL_LUMINANCE
    #undef GL_LUMINANCE
#endif
    #define GL_LUMINANCE GL_RED
    #define GL_TEXTURE_SWIZZLE_RGBA 0x8E46
#endif

#ifdef OPENGL_ES
    #define MANUAL_MIPMAP_GENERATION

    #define GL_COMPRESSED_RGBA_S3TC_DXT1_EXT 0x83F1
    #define GL_COMPRESSED_RGBA_S3TC_DXT3_EXT 0x83F2
    #define GL_COMPRESSED_RGBA_S3TC_DXT5_EXT 0x83F3

  #ifndef GL_IMG_texture_compression_pvrtc
    #define GL_COMPRESSED_RGB_PVRTC_4BPPV1_IMG 0x8c00
    #define GL_COMPRESSED_RGB_PVRTC_2BPPV1_IMG 0x8c01
    #define GL_COMPRESSED_RGBA_PVRTC_4BPPV1_IMG 0x8c02
    #define GL_COMPRESSED_RGBA_PVRTC_2BPPV1_IMG 0x8c03
  #endif

    #define GL_COMPRESSED_RGB8_ETC2 0x9274
    #define GL_COMPRESSED_RGB8_PUNCHTHROUGH_ALPHA1_ETC2 0x9276
    #define GL_COMPRESSED_RGBA8_ETC2_EAC 0x9278

  #ifdef __APPLE__
    #define GL_ETC1_RGB8_OES GL_COMPRESSED_RGB8_ETC2
  #else
    #define GL_ETC1_RGB8_OES 0x8D64
    #define GL_RED_EXT 0x1903
  #endif
#endif

#if !defined OPENGL_ES || defined __APPLE__
    #define USE_BGRA
#endif

#ifndef GL_MULTISAMPLE
    #define GL_MULTISAMPLE GL_MULTISAMPLE_ARB
#endif

#ifdef OPENGL_ES
    #ifdef __APPLE__
        #define glRenderbufferStorageMultisample glRenderbufferStorageMultisampleAPPLE
    #else
        typedef void (*PFNGLBLITFRAMEBUFFERPROC)(GLint,GLint,GLint,GLint,GLint,GLint,GLint,GLint,GLbitfield,GLenum);
        typedef void (*PFNGLREADBUFFERPROC) (GLenum);
        typedef void (*PFNGLRENDERBUFFERSTORAGEMULTISAMPLEPROC)(GLenum,GLsizei,GLenum,GLsizei,GLsizei);
        PFNGLBLITFRAMEBUFFERPROC glBlitFramebuffer=0;
        PFNGLREADBUFFERPROC glReadBuffer=0;
        PFNGLRENDERBUFFERSTORAGEMULTISAMPLEPROC glRenderbufferStorageMultisample=0;
        #define GL_READ_FRAMEBUFFER 0x8CA8
        #define GL_DRAW_FRAMEBUFFER 0x8CA9
    #endif
#endif

#ifndef GL_MAX_COLOR_ATTACHMENTS
    #define GL_MAX_COLOR_ATTACHMENTS 0x8CDF
#endif

#ifndef GL_MAX_SAMPLES
    #define GL_MAX_SAMPLES 0x8D57
#endif

    bool has_extension(const char *name)
    {
        const char *exts=(const char*)glGetString(GL_EXTENSIONS);
        if(!exts)
            return false;

        return strstr(exts,name)!=0;
    }

    void *get_extension(const char*ext_name)
    {
        if(!ext_name)
            return 0;

    #if defined __APPLE__
        return 0;
    #else
      #if defined OPENGL_ES
        return (void*)eglGetProcAddress(ext_name);
      #elif defined _WIN32
        return (void*)wglGetProcAddress(ext_name);
      #else
        return (void*)glXGetProcAddressARB((const GLubyte *)ext_name);
      #endif
    #endif
    }

#ifndef NO_EXTENSIONS_INIT
    PFNGLDELETESHADERPROC glDeleteShader=NULL;
    PFNGLDETACHSHADERPROC glDetachShader=NULL;
    PFNGLCREATESHADERPROC glCreateShader=NULL;
    PFNGLSHADERSOURCEPROC glShaderSource=NULL;
    PFNGLCOMPILESHADERPROC glCompileShader=NULL;
    PFNGLCREATEPROGRAMPROC glCreateProgram=NULL;
    PFNGLATTACHSHADERPROC glAttachShader=NULL;
    PFNGLLINKPROGRAMPROC glLinkProgram=NULL;
    PFNGLUSEPROGRAMPROC glUseProgram=NULL;
    PFNGLVALIDATEPROGRAMPROC glValidateProgram=NULL;
    PFNGLUNIFORM1IPROC glUniform1i=NULL;
    PFNGLUNIFORM1FVPROC glUniform1fv=NULL;
    PFNGLUNIFORM2FVPROC glUniform2fv=NULL;
    PFNGLUNIFORM3FVPROC glUniform3fv=NULL;
    PFNGLUNIFORM4FVPROC glUniform4fv=NULL;
    PFNGLUNIFORMMATRIX4FVPROC glUniformMatrix4fv=NULL;
    PFNGLGETPROGRAMIVPROC glGetProgramiv=NULL;
    PFNGLGETSHADERIVPROC glGetShaderiv=NULL;
    PFNGLGETPROGRAMINFOLOGPROC glGetProgramInfoLog=NULL;
    PFNGLGETSHADERINFOLOGPROC glGetShaderInfoLog=NULL;
    PFNGLGETUNIFORMLOCATIONPROC glGetUniformLocation=NULL;
    PFNGLBINDATTRIBLOCATIONPROC glBindAttribLocation=NULL;

    PFNGLBINDBUFFERBASEEXTPROC glBindBufferBase=NULL;
    PFNGLBINDBUFFERRANGEEXTPROC glBindBufferRange=NULL;
    PFNGLBEGINTRANSFORMFEEDBACKEXTPROC glBeginTransformFeedback=NULL;
    PFNGLENDTRANSFORMFEEDBACKEXTPROC glEndTransformFeedback=NULL;
    PFNGLTRANSFORMFEEDBACKVARYINGSPROC glTransformFeedbackVaryings=NULL;

    PFNGLGENFRAMEBUFFERSPROC glGenFramebuffers=NULL;
    PFNGLBINDFRAMEBUFFERPROC glBindFramebuffer=NULL;
    PFNGLDELETEFRAMEBUFFERSPROC glDeleteFramebuffers=NULL;
    PFNGLFRAMEBUFFERTEXTURE2DPROC glFramebufferTexture2D=NULL;

    PFNGLBLITFRAMEBUFFERPROC glBlitFramebuffer=NULL;
    PFNGLGENRENDERBUFFERSPROC glGenRenderbuffers=NULL;
    PFNGLBINDRENDERBUFFERPROC glBindRenderbuffer=NULL;
    PFNGLDELETERENDERBUFFERSPROC glDeleteRenderbuffers=NULL;
    PFNGLRENDERBUFFERSTORAGEMULTISAMPLEPROC glRenderbufferStorageMultisample=NULL;
    PFNGLFRAMEBUFFERRENDERBUFFERPROC glFramebufferRenderbuffer=NULL;

    PFNGLGENBUFFERSARBPROC glGenBuffers=NULL;
    PFNGLBINDBUFFERARBPROC glBindBuffer=NULL;
    PFNGLBUFFERDATAARBPROC glBufferData=NULL;
    PFNGLBUFFERSUBDATAARBPROC glBufferSubData=NULL;
    PFNGLGETBUFFERSUBDATAARBPROC glGetBufferSubData=NULL;
    PFNGLDELETEBUFFERSARBPROC glDeleteBuffers=NULL;

    PFNGLDRAWELEMENTSINSTANCEDARBPROC glDrawElementsInstancedARB=NULL;
    PFNGLDRAWARRAYSINSTANCEDARBPROC glDrawArraysInstancedARB=NULL;

    PFNGLVERTEXATTRIBPOINTERPROC glVertexAttribPointer=NULL;
    PFNGLENABLEVERTEXATTRIBARRAYPROC glEnableVertexAttribArray=NULL;
    PFNGLDISABLEVERTEXATTRIBARRAYPROC glDisableVertexAttribArray=NULL;

  #ifdef USE_VAO
    PFNGLBINDVERTEXARRAYPROC glBindVertexArray=NULL;
    PFNGLGENVERTEXARRAYSPROC glGenVertexArrays=NULL;
    PFNGLDELETEVERTEXARRAYSPROC glDeleteVertexArrays=NULL;
  #endif

#ifdef _WIN32
    PFNGLACTIVETEXTUREARBPROC glActiveTexture=NULL;
    PFNGLCOMPRESSEDTEXIMAGE2DARBPROC glCompressedTexImage2D=NULL;
#endif
    PFNGLGENERATEMIPMAPPROC glGenerateMipmap=NULL;

    PFNGLBLENDFUNCSEPARATEPROC glBlendFuncSeparate=NULL;

    PFNGLDEBUGMESSAGECALLBACKARBPROC glDebugMessageCallback=NULL;
#endif

    static void init_extensions()
    {
        static bool initialised=false;
        if(initialised)
            return;
        initialised=true;
    #ifndef NO_EXTENSIONS_INIT
        glDeleteShader         =(PFNGLDELETESHADERPROC)         get_extension("glDeleteShader");
        glDetachShader         =(PFNGLDETACHSHADERPROC)         get_extension("glDetachShader");
        glCreateShader         =(PFNGLCREATESHADERPROC)         get_extension("glCreateShader");
        glShaderSource         =(PFNGLSHADERSOURCEPROC)         get_extension("glShaderSource");
        glCompileShader        =(PFNGLCOMPILESHADERPROC)        get_extension("glCompileShader");
        glCreateProgram        =(PFNGLCREATEPROGRAMPROC)        get_extension("glCreateProgram");
        glAttachShader         =(PFNGLATTACHSHADERPROC)         get_extension("glAttachShader");
        glLinkProgram          =(PFNGLLINKPROGRAMPROC)          get_extension("glLinkProgram");
        glUseProgram           =(PFNGLUSEPROGRAMPROC)           get_extension("glUseProgram" );
        glValidateProgram      =(PFNGLVALIDATEPROGRAMPROC)      get_extension("glValidateProgram" );
        glUniform1i            =(PFNGLUNIFORM1IPROC)            get_extension("glUniform1i");
        glUniform1fv           =(PFNGLUNIFORM1FVPROC)           get_extension("glUniform1fv");
        glUniform2fv           =(PFNGLUNIFORM2FVPROC)           get_extension("glUniform2fv");
        glUniform3fv           =(PFNGLUNIFORM3FVPROC)           get_extension("glUniform3fv");
        glUniform4fv           =(PFNGLUNIFORM4FVPROC)           get_extension("glUniform4fv");
        glUniformMatrix4fv     =(PFNGLUNIFORMMATRIX4FVPROC)     get_extension("glUniformMatrix4fv");
        glGetProgramiv         =(PFNGLGETPROGRAMIVPROC)         get_extension("glGetProgramiv");
        glGetShaderiv          =(PFNGLGETSHADERIVPROC)          get_extension("glGetShaderiv");
        glGetShaderInfoLog     =(PFNGLGETSHADERINFOLOGPROC)     get_extension("glGetShaderInfoLog");
        glGetProgramInfoLog    =(PFNGLGETPROGRAMINFOLOGPROC)    get_extension("glGetProgramInfoLog");
        glGetUniformLocation   =(PFNGLGETUNIFORMLOCATIONPROC)   get_extension("glGetUniformLocation");
        glBindAttribLocation   =(PFNGLBINDATTRIBLOCATIONPROC)   get_extension("glBindAttribLocation");

        glGenFramebuffers=(PFNGLGENFRAMEBUFFERSPROC)get_extension("glGenFramebuffers");
        glBindFramebuffer=(PFNGLBINDFRAMEBUFFERPROC)get_extension("glBindFramebuffer");
        glDeleteFramebuffers=(PFNGLDELETEFRAMEBUFFERSPROC)get_extension("glDeleteFramebuffers");
        glFramebufferTexture2D=(PFNGLFRAMEBUFFERTEXTURE2DPROC)get_extension("glFramebufferTexture2D");

        glGenBuffers=(PFNGLGENBUFFERSARBPROC)get_extension("glGenBuffersARB");
        glBindBuffer=(PFNGLBINDBUFFERARBPROC)get_extension("glBindBufferARB");
        glBufferData=(PFNGLBUFFERDATAARBPROC)get_extension("glBufferDataARB");
        glBufferSubData=(PFNGLBUFFERSUBDATAARBPROC)get_extension("glBufferSubDataARB");
        glGetBufferSubData=(PFNGLGETBUFFERSUBDATAARBPROC)get_extension("glGetBufferSubDataARB");
        glDeleteBuffers=(PFNGLDELETEBUFFERSARBPROC)get_extension("glDeleteBuffersARB");

        glVertexAttribPointer=(PFNGLVERTEXATTRIBPOINTERPROC)get_extension("glVertexAttribPointer");
        glEnableVertexAttribArray=(PFNGLENABLEVERTEXATTRIBARRAYPROC)get_extension("glEnableVertexAttribArray");
        glDisableVertexAttribArray=(PFNGLDISABLEVERTEXATTRIBARRAYPROC)get_extension("glDisableVertexAttribArray");

    #ifdef USE_VAO
        glBindVertexArray=(PFNGLBINDVERTEXARRAYPROC)get_extension("glBindVertexArray");
        glGenVertexArrays=(PFNGLGENVERTEXARRAYSPROC)get_extension("glGenVertexArrays");
        glDeleteVertexArrays=(PFNGLDELETEVERTEXARRAYSPROC)get_extension("glDeleteVertexArrays");
    #endif

        if(has_extension("GL_ARB_draw_instanced"))
        {
            glDrawElementsInstancedARB=(PFNGLDRAWELEMENTSINSTANCEDARBPROC)get_extension("glDrawElementsInstancedARB");
            glDrawArraysInstancedARB=(PFNGLDRAWARRAYSINSTANCEDARBPROC)get_extension("glDrawArraysInstancedARB");
        }

        if(has_extension("GL_EXT_transform_feedback"))
        {
            glBindBufferBase=(PFNGLBINDBUFFERBASEEXTPROC)get_extension("glBindBufferBase");
            glBindBufferRange=(PFNGLBINDBUFFERRANGEEXTPROC)get_extension("glBindBufferRange");
            glBeginTransformFeedback=(PFNGLBEGINTRANSFORMFEEDBACKEXTPROC)get_extension("glBeginTransformFeedback");
            glEndTransformFeedback=(PFNGLENDTRANSFORMFEEDBACKEXTPROC)get_extension("glEndTransformFeedback");
            glTransformFeedbackVaryings=(PFNGLTRANSFORMFEEDBACKVARYINGSPROC)get_extension("glTransformFeedbackVaryings");
        }

#ifdef _WIN32
        glCompressedTexImage2D=(PFNGLCOMPRESSEDTEXIMAGE2DARBPROC)get_extension("glCompressedTexImage2DARB");
        glActiveTexture=(PFNGLACTIVETEXTUREARBPROC)get_extension("glActiveTextureARB");
#endif
        glGenerateMipmap=(PFNGLGENERATEMIPMAPPROC)get_extension("glGenerateMipmap");
        glBlendFuncSeparate=(PFNGLBLENDFUNCSEPARATEPROC)get_extension("glBlendFuncSeparate");

        if(has_extension("GL_ARB_debug_output"))
            glDebugMessageCallback=(PFNGLDEBUGMESSAGECALLBACKARBPROC)get_extension("glDebugMessageCallbackARB");

        glBlitFramebuffer=(PFNGLBLITFRAMEBUFFERPROC)get_extension("glBlitFramebuffer");
        glGenRenderbuffers=(PFNGLGENRENDERBUFFERSPROC)get_extension("glGenRenderbuffers");
        glBindRenderbuffer=(PFNGLBINDRENDERBUFFERPROC)get_extension("glBindRenderbuffer");
        glDeleteRenderbuffers=(PFNGLDELETERENDERBUFFERSPROC)get_extension("glDeleteRenderbuffers");
        glRenderbufferStorageMultisample=(PFNGLRENDERBUFFERSTORAGEMULTISAMPLEPROC)get_extension("glRenderbufferStorageMultisample");
        glFramebufferRenderbuffer=(PFNGLFRAMEBUFFERRENDERBUFFERPROC)get_extension("glFramebufferRenderbuffer");
    #endif

    #if defined OPENGL_ES && !defined __APPLE__
        const char *gl_version=(const char *)glGetString(GL_VERSION);
        const bool es3=gl_version!=0 && strncmp(gl_version,"OpenGL ES 3.",12)==0;
        if(es3)
        {
            glBlitFramebuffer=(PFNGLBLITFRAMEBUFFERPROC)get_extension("glBlitFramebuffer");
            glReadBuffer=(PFNGLREADBUFFERPROC)get_extension("glReadBuffer");
            glRenderbufferStorageMultisample=(PFNGLRENDERBUFFERSTORAGEMULTISAMPLEPROC)get_extension("glRenderbufferStorageMultisample");
        }
    #endif
    }
}
