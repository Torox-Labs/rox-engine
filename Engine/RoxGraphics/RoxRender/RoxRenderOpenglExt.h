// Updated By the ROX_ENGINE
// Copyright (C) 2024 Torox Project
// Portions Copyright (C) 2013 nyan.developer@gmail.com (nya-engine)
//
// This file was modified by the Torox Project.
// 
// This file incorporates code from the nya-engine project, which is licensed under the MIT License.
// See the LICENSE-MIT file in the root directory for more information.
//
// This file is also part of the Rox-engine, which is licensed under a dual-license system:
// 1. Free Use License (for non-commercial and commercial use under specific conditions)
// 2. Commercial License (for use on proprietary platforms)
// See the LICENSE file in the root directory for the full Rox-engine license terms.


#pragma once

#ifdef _WIN32
#define NOMINMAX
#include <windows.h>
#include <glad/include/glad/glad.h>
//#include <gl/gl.h>
//#include <gl/glext.h>

#pragma region From <wglext.h>

#define WGL_CONTEXT_MAJOR_VERSION_ARB		0x2091
#define WGL_CONTEXT_MINOR_VERSION_ARB		0x2092
#define WGL_CONTEXT_FLAGS_ARB				0x2094
#define WGL_CONTEXT_CORE_PROFILE_BIT_ARB	0x00000001
#define WGL_CONTEXT_PROFILE_MASK_ARB		0x9126

#define GL_RGB32F_ARB                     0x8815
#define GL_RGBA32F_ARB                    0x8814

#define GL_COMPRESSED_RGBA_S3TC_DXT1_EXT  0x83F1
#define GL_COMPRESSED_RGBA_S3TC_DXT3_EXT  0x83F2
#define GL_COMPRESSED_RGBA_S3TC_DXT5_EXT  0x83F3

#define GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT 0x84FF
#define GL_TEXTURE_MAX_ANISOTROPY_EXT     0x84FE

// Declare Create Context Attribs function pointer
typedef HGLRC(WINAPI* PFNWGLCREATECONTEXTATTRIBSARBPROC)(
    HDC,      			 // First parameter: Device Context
    HGLRC,				// Second parameter: Share Context
    const int* attribs // Third parameter: Context Attributes
    );
typedef void (APIENTRYP PFNGLBINDBUFFERBASEEXTPROC) (GLenum target, GLuint index, GLuint buffer);
typedef void (APIENTRYP PFNGLBINDBUFFERRANGEEXTPROC) (GLenum target, GLuint index, GLuint buffer, GLintptr offset, GLsizeiptr size);
typedef void (APIENTRYP PFNGLBEGINTRANSFORMFEEDBACKEXTPROC) (GLenum primitiveMode);
typedef void (APIENTRYP PFNGLENDTRANSFORMFEEDBACKEXTPROC) (void);

typedef void (APIENTRYP PFNGLGENBUFFERSARBPROC) (GLsizei n, GLuint* buffers);
typedef void (APIENTRYP PFNGLBINDBUFFERARBPROC) (GLenum target, GLuint buffer);
typedef void (APIENTRYP PFNGLBUFFERDATAARBPROC) (GLenum target, GLsizeiptrARB size, const GLvoid* data, GLenum usage);
typedef void (APIENTRYP PFNGLBUFFERSUBDATAARBPROC) (GLenum target, GLintptrARB offset, GLsizeiptrARB size, const GLvoid* data);
typedef void (APIENTRYP PFNGLGETBUFFERSUBDATAARBPROC) (GLenum target, GLintptrARB offset, GLsizeiptrARB size, GLvoid* data);
typedef void (APIENTRYP PFNGLDELETEBUFFERSARBPROC) (GLsizei n, const GLuint* buffers);

typedef void (APIENTRYP PFNGLDRAWELEMENTSINSTANCEDARBPROC) (GLenum mode, GLsizei count, GLenum type, const GLvoid* indices, GLsizei primcount);
typedef void (APIENTRYP PFNGLDRAWARRAYSINSTANCEDARBPROC) (GLenum mode, GLint first, GLsizei count, GLsizei primcount);

typedef void (APIENTRYP PFNGLACTIVETEXTUREARBPROC) (GLenum texture);
typedef void (APIENTRYP PFNGLCOMPRESSEDTEXIMAGE2DARBPROC) (GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLint border, GLsizei imageSize, const GLvoid* data);
typedef void (APIENTRYP PFNGLDEBUGMESSAGECALLBACKARBPROC) (GLDEBUGPROCARB callback, const GLvoid* userParam);



#pragma endregion

#pragma region From <wgl.h>

// 
#define GL_LUMINANCE                      0x1909
#define GL_LUMINANCE_ALPHA                0x190A

#define GL_STACK_OVERFLOW                 0x0503
#define GL_STACK_UNDERFLOW                0x0504

// Extension string query function pointer type
typedef const char* (WINAPI* PFNWGLGETEXTENSIONSSTRINGEXTPROC) (void);
// VSync control function piointer types
typedef BOOL(WINAPI* PFNWGLSWAPINTERVALEXTPROC) (int);
typedef int (WINAPI* PFNWGLGETSWAPINTERVALEXTPROC) (void);

#pragma endregion

#else // Linux
#include <GL/glx.h>    // GLX is Linux's system for connecting OpenGL to the X Window System
#include <GL/gl.h>     // Core OpenGL header
#include <GL/glext.h>  // OpenGL extensions
#endif

namespace
{

#ifndef GL_VERTEX_SHADER
    #define GL_VERTEX_SHADER GL_VERTEX_SHADER_ARB
#endif

#ifndef GL_FRAGMENT_SHADER
    #define GL_FRAGMENT_SHADER GL_FRAGMENT_SHADER_ARB
#endif

#ifndef GL_INTERLEAVED_ATTRIBS
    #define GL_INTERLEAVED_ATTRIBS GL_INTERLEAVED_ATTRIBS_EXT
#endif

#if defined OPENGL3
    #define USE_VAO
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

#ifndef GL_MULTISAMPLE
    #define GL_MULTISAMPLE GL_MULTISAMPLE_ARB
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

    
      #if defined _WIN32
        return (void*)wglGetProcAddress(ext_name);
      #else
        return (void*)glXGetProcAddressARB((const GLubyte *)ext_name);
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
    }
}
