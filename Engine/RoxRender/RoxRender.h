//nya-engine (C) nyan.developer@gmail.com released under the MIT license (see LICENSE)

#pragma once

#include "RoxLogger/RoxLogger.h"
#include "RoxMath/RoxMatrix.h"

namespace RoxRender
{

void set_log(RoxLogger::RoxLoggerBase *l);
RoxLogger::RoxLoggerBase &log();

struct Rectangle
{
    int x,y,width,height;

    bool operator == (const Rectangle &other) const
    { return width==other.width && height==other.height && x==other.x && y==other.y; }
    bool operator != (const Rectangle &other) const { return !(*this==other); }
    Rectangle(): x(0),y(0),width(0),height(0) {}
};

void setViewport(int x,int y,int width,int height);
void setViewport(const Rectangle &r);
const Rectangle &getViewport();

struct Scissor
{
    static void enable(int x,int y,int w,int h);
    static void enable(const Rectangle &r);
    static void disable();

    static bool isEnabled();
    static const Rectangle &get();
};

void set_projection_matrix(const RoxMath::Matrix4 &mat);
void set_modelview_matrix(const RoxMath::Matrix4 &mat);
void set_orientation_matrix(const RoxMath::Matrix4 &mat);

const RoxMath::Matrix4 &get_projection_matrix();
const RoxMath::Matrix4 &get_modelview_matrix();
const RoxMath::Matrix4 &get_orientation_matrix();

void set_clear_color(float r,float g,float b,float a);
void set_clear_color(const RoxMath::Vector4 &c);
RoxMath::Vector4 get_clear_color();
void set_clear_depth(float value);
float get_clear_depth();
void clear(bool clear_color,bool clear_depth,bool clear_stencil=false);

struct blend
{
	enum MODE
    {
        ZERO,
        ONE,
        SRC_COLOR,
        INV_SRC_COLOR,
        SRC_ALPHA,
        INV_SRC_ALPHA,
        DST_COLOR,
        INV_DST_COLOR,
        DST_ALPHA,
        INV_DST_ALPHA
    };

    static void enable(MODE src,MODE dst);
    static void disable();
};

struct CullFace
{
	enum ORDER
	{
		CCW,
		CW
	};

	static void enable(ORDER o);
    static void disable();
};

struct DepthTest
{
	enum COMPARISON
	{
		NEVER,
		LESS,
		EQUAL,
		GREATER,
		NOT_LESS, //greater or equal
		NOT_EQUAL,
		NOT_GREATER, //less or equal
		ALLWAYS
	};

    static void enable(COMPARISON MODE);
    static void disable();
};

struct Zwrite
{
    static void enable();
    static void disable();
};

struct Color_Write
{
    static void enable();
    static void disable();
};

struct State
{
    bool blend;
    blend::MODE blend_src;
    blend::MODE blend_dst;
    void setBlend(bool blend, blend::MODE src = blend::ONE, blend::MODE dst = blend::ZERO)
    {
        this->blend = blend;
        blend_src = src;
        blend_dst = dst;
    }

    bool CullFace;
    CullFace::ORDER cull_order;
    void setCullFace(bool CullFace, CullFace::ORDER order = CullFace::CCW)
    {
        this->CullFace = CullFace;
        cull_order = order;
    }

    bool DepthTest;
    DepthTest::COMPARISON depth_comparsion;

    bool zwrite;
    bool color_write;

    State():
        blend(false),
        blend_src(blend::ONE),
        blend_dst(blend::ZERO),

        CullFace(false),
        cull_order(CullFace::CCW),

        DepthTest(true),
        depth_comparsion(DepthTest::NOT_GREATER),

        zwrite(true),
        color_write(true)
    {}
};

void setState(const State &s);
const State &get_state();

void applyState(bool ignore_cache=false);

//artificial restrictions for consistent behaviour among platforms
void setPlatformRestrictions(bool ignore);
bool isPlatformRestrictionsIgnored();
}
