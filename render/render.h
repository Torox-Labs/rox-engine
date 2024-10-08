//nya-engine (C) nyan.developer@gmail.com released under the MIT license (see LICENSE)

#pragma once

#include "log/log.h"
#include "math/matrix.h"

namespace nya_render
{

void set_log(nya_log::log_base *l);
nya_log::log_base &log();

struct rect
{
    int x,y,width,height;

    bool operator == (const rect &other) const
    { return width==other.width && height==other.height && x==other.x && y==other.y; }
    bool operator != (const rect &other) const { return !(*this==other); }
    rect(): x(0),y(0),width(0),height(0) {}
};

void set_viewport(int x,int y,int width,int height);
void set_viewport(const rect &r);
const rect &get_viewport();

struct scissor
{
    static void enable(int x,int y,int w,int h);
    static void enable(const rect &r);
    static void disable();

    static bool is_enabled();
    static const rect &get();
};

void set_projection_matrix(const nya_math::mat4 &mat);
void set_modelview_matrix(const nya_math::mat4 &mat);
void set_orientation_matrix(const nya_math::mat4 &mat);

const nya_math::mat4 &get_projection_matrix();
const nya_math::mat4 &get_modelview_matrix();
const nya_math::mat4 &get_orientation_matrix();

void set_clear_color(float r,float g,float b,float a);
void set_clear_color(const nya_math::vec4 &c);
nya_math::vec4 get_clear_color();
void set_clear_depth(float value);
float get_clear_depth();
void clear(bool clear_color,bool clear_depth,bool clear_stencil=false);

struct blend
{
    enum mode
    {
        zero,
        one,
        src_color,
        inv_src_color,
        src_alpha,
        inv_src_alpha,
        dst_color,
        inv_dst_color,
        dst_alpha,
        inv_dst_alpha
    };

    static void enable(mode src,mode dst);
    static void disable();
};

struct cull_face
{
    enum order
    {
        ccw,
        cw
    };

    static void enable(order o);
    static void disable();
};

struct depth_test
{
    enum comparsion
    {
        never,
        less,
        equal,
        greater,
        not_less, //greater or equal
        not_equal,
        not_greater, //less or equal
        allways
    };

    static void enable(comparsion mode);
    static void disable();
};

struct zwrite
{
    static void enable();
    static void disable();
};

struct color_write
{
    static void enable();
    static void disable();
};

struct state
{
    bool blend;
    blend::mode blend_src;
    blend::mode blend_dst;
    void set_blend(bool blend, blend::mode src = blend::one, blend::mode dst = blend::zero)
    {
        this->blend = blend;
        blend_src = src;
        blend_dst = dst;
    }

    bool cull_face;
    cull_face::order cull_order;
    void set_cull_face(bool cull_face, cull_face::order order = cull_face::ccw)
    {
        this->cull_face = cull_face;
        cull_order = order;
    }

    bool depth_test;
    depth_test::comparsion depth_comparsion;

    bool zwrite;
    bool color_write;

    state():
        blend(false),
        blend_src(blend::one),
        blend_dst(blend::zero),

        cull_face(false),
        cull_order(cull_face::ccw),

        depth_test(true),
        depth_comparsion(depth_test::not_greater),

        zwrite(true),
        color_write(true)
    {}
};

void set_state(const state &s);
const state &get_state();

void apply_state(bool ignore_cache=false);

//artificial restrictions for consistent behaviour among platforms
void set_platform_restrictions(bool ignore);
bool is_platform_restrictions_ignored();
}
