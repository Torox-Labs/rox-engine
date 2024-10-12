//nya-engine (C) nyan.developer@gmail.com released under the MIT license (see LICENSE)

#include "render_directx11.h"

namespace nya_render
{

bool render_directx11::is_available() const
{
#ifdef _WIN32
    return true;
#else
    return false;
#endif
}

namespace
{
    render_api_interface::state applied_state;
    bool ignore_cache = true;
}

/*
inline const state &get_current_state()
{
#ifdef DIRECTX11
    static state s;
    s=has_state_override?get_overriden_state(current_state):current_state;

    //flip cull order when flipping y
    if(!dx_is_default_target())
        s.cull_order=(s.cull_order==cull_face::ccw ? cull_face::cw : cull_face::ccw);
#else
    if(!has_state_override)
        return current_state;

    static state s;
    s=get_overriden_state(current_state);
#endif

    return s;
}
*/

#ifdef DIRECTX11

    class rasterizer_state_class
    {
        struct rdesc;

        ID3D11RasterizerState *get(const rdesc &d)
        {
            const unsigned int hsh=(d.cull?1:0) + (d.scissor?2:0) + d.cull_order*4;
            cache_map::iterator it=m_map.find(hsh);
            if(it!=m_map.end())
                return it->second;

            D3D11_RASTERIZER_DESC desc;
            ZeroMemory(&desc,sizeof(desc));
            if(d.cull)
            {
                if(d.cull_order==cull_face::cw)
                    desc.CullMode=D3D11_CULL_BACK;
                else
                    desc.CullMode=D3D11_CULL_FRONT;
            }
            else
                desc.CullMode=D3D11_CULL_NONE;

            desc.FillMode=D3D11_FILL_SOLID;
            desc.DepthClipEnable=true;
            desc.ScissorEnable=d.scissor;
            ID3D11RasterizerState *state;
            get_device()->CreateRasterizerState(&desc,&state);
            m_map[hsh]=state;
            return state;
        }

    public:
        void apply()
        {
            if(!get_device() || !get_context())
                return;

            auto &c=get_current_state();

            rdesc d;
            d.cull=c.cull_face;
            d.cull_order=c.cull_order;
            d.scissor=scissor_test;

            get_context()->RSSetState(get(d));
        }

        void release()
        {
            if(get_context())
                get_context()->RSSetState(0);

            for(auto &s:m_map) if(s.second) s.second->Release();
            m_map.clear();
        }

    private:
        struct rdesc
        {
            bool cull;
            cull_face::order cull_order;
            bool scissor;
        };

        typedef std::map<unsigned int,ID3D11RasterizerState*> cache_map;
        cache_map m_map;

    } rasterizer_state;

    namespace
    {

        D3D11_BLEND dx_blend_mode(blend::mode m)
        {
            switch(m)
            {
                case blend::zero: return D3D11_BLEND_ZERO;
                case blend::one: return D3D11_BLEND_ONE;
                case blend::src_color: return D3D11_BLEND_SRC_COLOR;
                case blend::inv_src_color: return D3D11_BLEND_INV_SRC_COLOR;
                case blend::src_alpha: return D3D11_BLEND_SRC_ALPHA;
                case blend::inv_src_alpha: return D3D11_BLEND_INV_SRC_ALPHA;
                case blend::dst_color: return D3D11_BLEND_DEST_COLOR;
                case blend::inv_dst_color: return D3D11_BLEND_INV_DEST_COLOR;
                case blend::dst_alpha: return D3D11_BLEND_DEST_ALPHA;
                case blend::inv_dst_alpha: return D3D11_BLEND_INV_DEST_ALPHA;
            }

            return D3D11_BLEND_ONE;
        }

        class
        {
        public:
            ID3D11BlendState *get(D3D11_BLEND src,D3D11_BLEND dst)
            {
                unsigned int hsh=src;
                hsh+=128*dst;

                cache_map::iterator it=m_map.find(hsh);
                if(it!=m_map.end())
                    return it->second;

                const D3D11_RENDER_TARGET_BLEND_DESC desc=
                {
                    true,
                    src,
                    dst,
                    D3D11_BLEND_OP_ADD,
                    D3D11_BLEND_ONE,
                    D3D11_BLEND_ONE,
                    D3D11_BLEND_OP_ADD,
                    D3D11_COLOR_WRITE_ENABLE_ALL
                };

                D3D11_BLEND_DESC blend_desc;
                blend_desc.AlphaToCoverageEnable=false;
                blend_desc.IndependentBlendEnable=false;
                blend_desc.RenderTarget[0]=desc;

                ID3D11BlendState *state;
                get_device()->CreateBlendState(&blend_desc,&state);
                m_map[hsh]=state;
                return state;
            }

            void apply()
            {
                if(!get_device() || !get_context())
                    return;

                auto &c=get_current_state();

                if(!c.blend)
                {
                    get_context()->OMSetBlendState(0,0,c.color_write?0xffffffff:0);
                    return;
                }

                ID3D11BlendState *state=get(dx_blend_mode(c.blend_src),
                                            dx_blend_mode(c.blend_dst));
                const float blend_factor[]={1.0f,1.0f,1.0f,1.0f};
                get_context()->OMSetBlendState(state,blend_factor,c.color_write?0xffffffff:0);
            }

            void release()
            {
                if(get_context())
                    get_context()->OMSetBlendState(0,0,0xffffffff);

                for(auto &s:m_map) if(s.second) s.second->Release();
                m_map.clear();
            }

        private:
            typedef std::map<unsigned int,ID3D11BlendState*> cache_map;
            cache_map m_map;

        } blend_state;

        class
        {
        public:
            ID3D11DepthStencilState *get(bool test,bool write,D3D11_COMPARISON_FUNC comparsion)
            {
                unsigned int hsh=0;
                if(test) hsh=1;
                if(write) hsh+=2;
                hsh+=4*comparsion;

                cache_map::iterator it=m_map.find(hsh);
                if(it!=m_map.end())
                    return it->second;

                D3D11_DEPTH_STENCIL_DESC desc;
                desc.DepthEnable=test;
                desc.DepthWriteMask=write?D3D11_DEPTH_WRITE_MASK_ALL:D3D11_DEPTH_WRITE_MASK_ZERO;
                desc.DepthFunc=comparsion;
                desc.StencilEnable=true;
                desc.StencilReadMask=0xFF;
                desc.StencilWriteMask=0xFF;

                desc.FrontFace.StencilFailOp=D3D11_STENCIL_OP_KEEP;
                desc.FrontFace.StencilDepthFailOp=D3D11_STENCIL_OP_INCR;
                desc.FrontFace.StencilPassOp=D3D11_STENCIL_OP_KEEP;
                desc.FrontFace.StencilFunc=D3D11_COMPARISON_ALWAYS;

                desc.BackFace.StencilFailOp=D3D11_STENCIL_OP_KEEP;
                desc.BackFace.StencilDepthFailOp=D3D11_STENCIL_OP_DECR;
                desc.BackFace.StencilPassOp=D3D11_STENCIL_OP_KEEP;
                desc.BackFace.StencilFunc=D3D11_COMPARISON_ALWAYS;

                ID3D11DepthStencilState *state;
                get_device()->CreateDepthStencilState(&desc,&state);
                m_map[hsh]=state;
                return state;
            }

            void apply()
            {
                if(!get_device() || !get_context())
                    return;

                auto &c=get_current_state();

                D3D11_COMPARISON_FUNC dx_depth_comparsion=D3D11_COMPARISON_ALWAYS;
                switch(c.depth_comparsion)
                {
                    case depth_test::never: dx_depth_comparsion=D3D11_COMPARISON_NEVER; break;
                    case depth_test::less: dx_depth_comparsion=D3D11_COMPARISON_LESS; break;
                    case depth_test::equal: dx_depth_comparsion=D3D11_COMPARISON_EQUAL; break;
                    case depth_test::greater: dx_depth_comparsion=D3D11_COMPARISON_GREATER; break;
                    case depth_test::not_less: dx_depth_comparsion=D3D11_COMPARISON_GREATER_EQUAL; break;
                    case depth_test::not_equal: dx_depth_comparsion=D3D11_COMPARISON_NOT_EQUAL; break;
                    case depth_test::not_greater: dx_depth_comparsion=D3D11_COMPARISON_LESS_EQUAL; break;
                    case depth_test::allways: dx_depth_comparsion=D3D11_COMPARISON_ALWAYS; break;
                }

                ID3D11DepthStencilState *state=get(c.depth_test,c.zwrite,dx_depth_comparsion);
                get_context()->OMSetDepthStencilState(state,1);
            }

            void release()
            {
                if(get_context())
                    get_context()->OMSetDepthStencilState(0,0);

                for(auto &s:m_map) if(s.second) s.second->Release();
                m_map.clear();
            }

        private:
            typedef std::map<unsigned int,ID3D11DepthStencilState*> cache_map;
            cache_map m_map;

        } depth_state;

    }

#endif

static void apply_viewport_state()
{
#ifdef DIRECTX11
    if(!get_context() || !get_device())
        return;

    const state &c=get_current_state();
    state &a=applied_state;

    const int h=dx_get_target_height();
    const int vp_y=dx_is_default_target()?(h-viewport_rect.y-viewport_rect.height):viewport_rect.y;
    if(viewport_rect.x!=viewport_applied_rect.x || vp_y!=viewport_applied_rect.y
       || viewport_rect.width!=viewport_applied_rect.width || viewport_rect.height!=viewport_applied_rect.height
       || ignore_cache)
    {
        D3D11_VIEWPORT vp;
        vp.Width=FLOAT(viewport_applied_rect.width=viewport_rect.width);
        viewport_applied_rect.height = int(vp.Height = (FLOAT)viewport_rect.height);
        vp.MinDepth = 0.0f;
        vp.MaxDepth = 1.0f;
        viewport_applied_rect.x = int(vp.TopLeftX = (FLOAT)viewport_rect.x);
        viewport_applied_rect.y = int(vp.TopLeftY = (FLOAT)vp_y);
        get_context()->RSSetViewports(1,&vp);
    }

    if(scissor_test || ignore_cache) //ToDo: cache
    {
        D3D11_RECT r;
        r.left=scissor_rect.x;
        r.right=scissor_rect.x+scissor_rect.width;
        r.top=dx_is_default_target()?(h-scissor_rect.y-scissor_rect.height):scissor_rect.y;
        r.bottom=h-scissor_rect.y;
        get_context()->RSSetScissorRects(1,&r);
    }

    if(c.cull_order!=a.cull_order || c.cull_face!=a.cull_face
       || scissor_test!=applied_scissor_test || ignore_cache)
    {
        rasterizer_state.apply();
        a.cull_order=c.cull_order;
        a.cull_face=c.cull_face;
        applied_scissor_test=scissor_test;
    }
#endif
}

namespace { nya_math::mat4 modelview, projection; }

void render_directx11::set_camera(const nya_math::mat4 &mv,const nya_math::mat4 &p)
{
    modelview=mv;
    projection=p;
}

void render_directx11::clear(const viewport_state &s,bool color,bool depth,bool stencil)
{

}

void render_directx11::invalidate_cached_state() { ignore_cache = true; }

void render_directx11::apply_state(const state &s)
{
    //ToDo

    applied_state = s;
    ignore_cache = false;
}

render_directx11 &render_directx11::get() { static render_directx11 *api = new render_directx11(); return *api; }

#ifdef DIRECTX11
namespace
{
    ID3D11Device *render_device=0;
    ID3D11DeviceContext *render_context=0;

    void discard_state()
    {
        invalidate_resources();
        applied_state=current_state=state();

        rasterizer_state=rasterizer_state_class();
        depth_state=decltype(depth_state)();
        blend_state=decltype(blend_state)();
    }

}

void release_states()
{
    depth_state.release();
    rasterizer_state.release();
    blend_state.release();
}

ID3D11Device *get_device() { return render_device; }
void set_device(ID3D11Device *device)
{
    if(render_device==device)
        return;

    discard_state();
    render_device=device;
}

ID3D11DeviceContext *get_context() { return render_context; }
void set_context(ID3D11DeviceContext *context)
{
    if(render_context==context)
        return;

    discard_state();
    render_context=context;
}

void render_directx11::set_default_target(ID3D11RenderTargetView *color,ID3D11DepthStencilView *depth,int height)
{
    dx_set_target(color,depth,true,height);
}
#endif

}
