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
    void draw(const state &s) override {}

public:
    void invalidateCachedState() override;
    void applyState(const state &s) override;

public:
    static RoxRenderDirectx11 &get();

private:
    RoxRenderDirectx11() {}
    
private:
    ID3D11Device *get_device();
    void set_device(ID3D11Device *device);

    ID3D11DeviceContext *get_context();
    void set_context(ID3D11DeviceContext *context);

    void set_default_target(ID3D11RenderTargetView *color,ID3D11DepthStencilView *depth,int height=-1);
};

}
