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

void setProjectionMatrix(const RoxMath::Matrix4 &mat);
void setModelviewMatrix(const RoxMath::Matrix4 &mat);
void setOrientationMatrix(const RoxMath::Matrix4 &mat);

const RoxMath::Matrix4 &getProjectionMatrix();
const RoxMath::Matrix4 &getModelviewMatrix();
const RoxMath::Matrix4 &getOrientationMatrix();

void setClearColor(float r,float g,float b,float a);
void setClearColor(const RoxMath::Vector4 &c);
RoxMath::Vector4 getClearColor();
void setClearDepth(float value);
float getClearDepth();
void clear(bool clear_color,bool clear_depth,bool clear_stencil=false);

struct Blend
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
    Blend::MODE blend_src;
    Blend::MODE blend_dst;
    void setBlend(bool blend, Blend::MODE src = Blend::ONE, Blend::MODE dst = Blend::ZERO)
    {
        this->blend = blend;
        blend_src = src;
        blend_dst = dst;
    }

    bool cull_face;
    CullFace::ORDER cull_order;
    void setCullFace(bool CullFace, CullFace::ORDER order = CullFace::CCW)
    {
        this->cull_face = CullFace;
        cull_order = order;
    }

    bool depth_test;
    DepthTest::COMPARISON depth_comparsion;

    bool zwrite;
    bool color_write;

    State():
        blend(false),
        blend_src(Blend::ONE),
        blend_dst(Blend::ZERO),

        cull_face(false),
        cull_order(CullFace::CCW),

        depth_test(true),
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
