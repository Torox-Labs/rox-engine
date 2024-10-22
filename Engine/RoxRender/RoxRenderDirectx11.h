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

#include "RoxRenderApi.h"

struct ID3D11Device;
struct ID3D11DeviceContext;
struct ID3D11RenderTargetView;
struct ID3D11DepthStencilView;

namespace RoxRender
{

class RoxRenderDirectx11: public RoxRenderApiInterface
{
public:
    bool isAvailable() const override;

public:
    void setCamera(const RoxMath::Matrix4 &mv,const RoxMath::Matrix4 &p) override;
    void clear(const ViewportState &s,bool color,bool depth,bool stencil) override;
    void draw(const State &s) override {}

public:
    void invalidateCachedState() override;
    void applyState(const State &s) override;

public:
    static RoxRenderDirectx11 &get();

private:
    RoxRenderDirectx11() {}
    
private:
    ID3D11Device *getDevice();
    void setDevice(ID3D11Device *device);

    ID3D11DeviceContext *getContext();
    void setContext(ID3D11DeviceContext *context);

    void setDefaultTarget(ID3D11RenderTargetView *color,ID3D11DepthStencilView *depth,int height=-1);
};

}
