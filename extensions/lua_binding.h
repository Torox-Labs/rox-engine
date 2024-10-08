//nya-engine (C) nyan.developer@gmail.com released under the MIT license (see LICENSE)

#pragma once

#include "math/quaternion.h"
#include "log/log.h"

struct lua_State;

namespace nya_math
{
    void bind_to_lua(lua_State *state);

    const vec2 *lua_testvec2(lua_State *state,int idx);
    const vec3 *lua_testvec3(lua_State *state,int idx);
    const vec4 *lua_testvec4(lua_State *state,int idx);
    const quat *lua_testquat(lua_State *state,int idx);
    const mat4 *lua_testmat4(lua_State *state,int idx);

    void lua_pushvec2(lua_State *state,const vec2 &v);
    void lua_pushvec3(lua_State *state,const vec3 &v);
    void lua_pushvec4(lua_State *state,const vec4 &v);
    void lua_pushquat(lua_State *state,const quat &q);
    void lua_pushmat4(lua_State *state,const mat4 &m);
}

namespace nya_log
{
    void set_lua_log(lua_State *state,log_base *l=0,const char *prefix=0);
}
