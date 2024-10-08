#pragma once

#include "btBulletDynamicsCommon.h"
#include "render/debug_draw.h"
#include "render/render.h"
#include "scene/camera.h"

class bullet_debug_draw: public btIDebugDraw
{
public:
    void set_debug_mode(int mode) { setDebugMode(mode); }
    void set_alpha(float alpha) { m_alpha=alpha; }
    void set_scale(float scale) { m_scale = scale; }
    void release() { m_dd.release(); }

    bullet_debug_draw(): m_debug_mode(0),m_alpha(1.0f) {}

protected:
    void flushLines()
    {
        nya_render::state state;
        state.depth_test=false;
        nya_render::set_state(state);
        nya_render::set_modelview_matrix(nya_scene::get_camera().get_view_matrix());
        if(m_alpha<1.0f)
            nya_render::blend::enable(nya_render::blend::src_alpha,nya_render::blend::inv_src_alpha);
        m_dd.draw();
        m_dd.clear();
    }

    void drawLine(const btVector3 &from,const btVector3 &to,const btVector3 &color)
    {
        m_dd.add_line(nya_math::vec3(from)*m_scale,nya_math::vec3(to)*m_scale,nya_math::vec4(nya_math::vec3(color),m_alpha));
    }

    int	getDebugMode() const { return m_debug_mode; }
    void setDebugMode(int mode) { m_debug_mode=mode; }

private:
    void drawContactPoint(const btVector3 &,const btVector3 &,btScalar,int,const btVector3 &) {}
    void reportErrorWarning(const char*) {}
    void draw3dText(const btVector3 &,const char*) {}

protected:
    nya_render::debug_draw m_dd;
    int m_debug_mode;
    float m_alpha;
    float m_scale = 1.0f;
};
