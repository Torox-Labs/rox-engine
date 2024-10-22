//nya-engine (C) nyan.developer@gmail.com released under the MIT license (see LICENSE)

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
