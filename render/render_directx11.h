//nya-engine (C) nyan.developer@gmail.com released under the MIT license (see LICENSE)

#pragma once

#include "render_api.h"

struct ID3D11Device;
struct ID3D11DeviceContext;
struct ID3D11RenderTargetView;
struct ID3D11DepthStencilView;

namespace nya_render
{

class render_directx11: public render_api_interface
{
public:
    bool is_available() const override;

public:
    void set_camera(const nya_math::mat4 &mv,const nya_math::mat4 &p) override;
    void clear(const viewport_state &s,bool color,bool depth,bool stencil) override;
    void draw(const state &s) override {}

public:
    void invalidate_cached_state() override;
    void apply_state(const state &s) override;

public:
    static render_directx11 &get();

private:
    render_directx11() {}
    
private:
    ID3D11Device *get_device();
    void set_device(ID3D11Device *device);

    ID3D11DeviceContext *get_context();
    void set_context(ID3D11DeviceContext *context);

    void set_default_target(ID3D11RenderTargetView *color,ID3D11DepthStencilView *depth,int height=-1);
};

}
