// Updated By the ROX_ENGINE
// Copyright (C) 2024 Torox Project
// Portions Copyright (C) 2013 nyan.developer@gmail.com (nya-engine)
//
// This file was modified by the Torox Project.
// Update the render api intefrace to check Metal 1th.
//
// This file incorporates code from the nya-engine project, which is licensed under the MIT License.
// See the LICENSE-MIT file in the root directory for more information.
//
// This file is also part of the Rox-engine, which is licensed under a dual-license system:
// 1. Free Use License (for non-commercial and commercial use under specific conditions)
// 2. Commercial License (for use on proprietary platforms)
// See the LICENSE file in the root directory for the full Rox-engine license terms.

#pragma once

#include "RoxRenderApi.h"

namespace RoxRender
{

class RoxRenderOpengl: public RoxRenderApiInterface
{
public:
    bool isAvailable() const override;

public:
    void invalidateCachedState() override;
    void applyState(const State &s) override;

public:
    int createShader(const char *vertex,const char *fragment) override;
    uint getUniformsCount(int shader) override;
    RoxShader::uniform getUniform(int shader,int idx) override;
    void removeShader(int shader) override;

    int createUniformBuffer(int shader) override;
    void setUniform(int uniform_buffer,int idx,const float *buf,uint count) override;
    void removeUniformBuffer(int uniform_buffer) override;

public:
    int createVertexBuffer(const void *data,uint stride,uint count,RoxVbo::USAGE_HINT usage) override;
    void setVertexLayout(int idx,RoxVbo::Layout Layout) override;
    void updateVertexBuffer(int idx,const void *data) override;
    bool getVertexData(int idx,void *data) override;
    void removeVertexBuffer(int idx) override;

    int createIndexBuffer(const void *data,RoxVbo::INDEX_SIZE size,uint indices_count,RoxVbo::USAGE_HINT usage) override;
    void updateIndexBuffer(int idx,const void *data) override;
    bool getIndexData(int idx,void *data) override;
    void removeIndexBuffer(int idx) override;

public:
    int createTexture(const void *data,uint width,uint height,RoxTexture::COLOR_FORMAT &format,int mip_count) override;
    int createCubemap(const void *data[6],uint width,RoxTexture::COLOR_FORMAT &format,int mip_count) override;
    void updateTexture(int idx,const void *data,uint x,uint y,uint width,uint height,int mip) override;
    void setTextureWrap(int idx,RoxTexture::WRAP s,RoxTexture::WRAP t) override;
    void setTextureFilter(int idx,RoxTexture::FILTER minification,RoxTexture::FILTER magnification,RoxTexture::FILTER mipmap,uint aniso) override;
    bool getTextureData(int RoxTexture,uint x,uint y,uint w,uint h,void *data) override;
    void removeTexture(int RoxTexture) override;
    uint getMaxTextureDimention() override;
    bool isTextureFormatSupported(RoxTexture::COLOR_FORMAT format) override;

public:
    int createTarget(uint width,uint height,uint samples,const int *attachment_textures,
                      const int *attachment_sides,uint attachment_count,int depth_texture) override;
    void resolveTarget(int idx) override;
    void removeTarget(int idx) override;
    uint getMaxTargetAttachments() override;
    uint getMaxTargetMsaa() override;

public:
    void setCamera(const RoxMath::Matrix4 &modelview,const RoxMath::Matrix4 &projection) override;
    void clear(const ViewportState &s,bool color,bool depth,bool stencil) override;
    void draw(const State &s) override;
    void transformFeedback(const TfState &s) override;
    bool isTransformFeedbackSupported() override;

public:
    static void logErrors(const char *place=0);
    static void enableDebug(bool synchronous);

    static bool hasExtension(const char *name);
    static void *getExtension(const char *name);

    uint get_glTexture_id(int idx);

    //for external textures
    static void glBindTexture(uint gl_type,uint gl_idx,uint layer=0);
    static void glBindTexture2d(uint gl_idx,uint layer=0);

public:
    static RoxRenderOpengl &get();

private:
    RoxRenderOpengl() {}
};

}
