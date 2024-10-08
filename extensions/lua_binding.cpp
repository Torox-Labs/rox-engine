//nya-engine (C) nyan.developer@gmail.com released under the MIT license (see LICENSE)

//binding for lua https://www.lua.org/

#include "lua_binding.h"
#include "math/matrix.h"
#include <vector>

extern "C"
{
#include "lua.h"
#include "lauxlib.h"
#include "lualib.h"
}

namespace
{

template <typename t> struct table
{
    static std::string &name() { static std::string s; return s; }
    static std::string &lname() { static std::string s; return s; }
    static std::string &components() { static std::string s; return s; }
    static std::vector<luaL_Reg> &methods() { static std::vector<luaL_Reg> m; return m; }

    static void bind(lua_State *state,const char *name_, const char * components_, luaL_Reg regs[],lua_CFunction ctor)
    {
        name()=name_;
        lname()="luaL_"+name();
        if(components_)
            components()=components_;

        methods().clear();
        luaL_newmetatable(state,lname().c_str());
        for(luaL_Reg *m=regs;m->name;++m)
        {
            if(m->name[0]=='_' && m->name[1]!='_')
            {
                methods().push_back(*m);
                continue;
            }

            lua_pushcfunction(state,m->func);
            lua_setfield(state,-2,m->name);
        }
        lua_pushcfunction(state,gc);
        lua_setfield(state,-2,"__gc");

        lua_pushcfunction(state,getter);
        lua_setfield(state,-2,"__index");
        lua_pushcfunction(state,setter);
        lua_setfield(state,-2,"__newindex");

        if(!components().empty())
        {
            lua_pushcfunction(state,tostring);
            lua_setfield(state,-2,"__tostring");
        }

        lua_newtable(state);
        lua_pushcfunction(state,ctor);
        lua_setfield(state,-2,"__call");
        lua_setmetatable(state,-2);
        lua_setglobal(state,name().c_str());
    }

    static t *test(lua_State *state,int idx)
    {
        t** result=(t**)luaL_testudata(state,idx,lname().c_str());
        return result?*result:0;
    }

    static t &push(lua_State *state)
    {
        t** udata = (t**)lua_newuserdata(state,sizeof(t*));
        *udata = new t();
        luaL_getmetatable(state,lname().c_str());
        lua_setmetatable(state,-2);
        return **udata;
    }

    static int getter(lua_State *state)
    {
        t *v=test(state,1);
        const char *n=luaL_checkstring(state,2);
        if(n[1])
        {
            for(int i=0;i<(int)methods().size();++i)
            {
                if(strcmp(n,methods()[i].name+1)==0)
                {
                    lua_pushcfunction(state,methods()[i].func);
                    return 1;
                }
            }
            return 0;
        }

        for(int i=0;i<components().length();++i)
        {
            if(n[0]==components()[i])
            {
                lua_pushnumber(state,((float *)v)[i]);
                return 1;
            }
        }
        return 0;
    }

    static int setter(lua_State *state)
    {
        t *v=test(state,1);
        const char *n=luaL_checkstring(state,2);
        if(n[1])
            return 0;

        for(int i=0;i<components().length();++i)
        {
            if(n[0]==components()[i])
            {
                ((float *)v)[i]=luaL_checknumber(state,3);
                break;
            }
        }
        return 0;
    }

    static int tostring(lua_State *state)
    {
        t *v=test(state,1);
        std::string result="(";
        char buf[16];
        for(int i=0;i<components().length();++i)
        {
            sprintf(buf,"%f,",((float *)v)[i]);
            result+=buf;
        }
        result[result.length()-1]=')';
        lua_pushstring(state,result.c_str());
        return 1;
    }

    static int gc(lua_State *state) { delete test(state,1); return 0; }
};

struct lua_args
{
    lua_State *state;
    const int count;

    lua_args(lua_State *s):state(s),count(lua_gettop(s)) {}

    template<typename t> bool is_type(int idx)
    {
        if(idx>=count || lua_type(state,idx+1)!=LUA_TUSERDATA)
            return false;
        return get_p<t>(idx)!=0;
    }

    template<typename t> t *get_p(int idx) { return idx<count?table<t>::test(state, idx+1):0; }
    template<typename t> t get(int idx) { t *r=get_p<t>(idx); return r?*r:t(); }

    template<typename t> int ret(const t &value) { table<t>::push(state)=value; return 1; }
};

template<> bool lua_args::is_type<float>(int idx) { return idx<count?lua_type(state,idx+1)==LUA_TNUMBER:false; }

template <> float lua_args::get<float>(int idx) { return idx<count?(float)luaL_checknumber(state,idx+1):0.0f; }
template <> bool lua_args::get<bool>(int idx) { return idx<count && lua_isboolean(state,idx+1)?lua_toboolean(state,idx+1)>0:false; }
template <> const char *lua_args::get<const char *>(int idx) { return idx<count?luaL_checkstring(state,idx+1):""; }

template <> int lua_args::ret<float>(const float &value) { lua_pushnumber(state,value); return 1; }

}

namespace nya_math
{

namespace
{
    template<typename t> int vec_new(lua_State *state)
    {
        lua_remove(state,1);
        lua_args args(state);
        t r;
        for(int i=0;i<int(sizeof(t)/sizeof(float)) && i<args.count;++i)
            ((float *)&r)[i]=args.get<float>(i);
        return args.ret(r);
    }

    template<typename t> int vec_unm(lua_State *state) { lua_args args(state); return args.ret(-args.get<t>(0)); }
    template<typename t> int vec_add(lua_State *state) { lua_args args(state); return args.ret(args.get<t>(0) + args.get<t>(1)); }
    template<typename t> int vec_sub(lua_State *state) { lua_args args(state); return args.ret(args.get<t>(0) - args.get<t>(1)); }

    template<typename t> int vec_mul(lua_State *state)
    {
        lua_args args(state);
        if(args.is_type<float>(0))
            return args.ret(args.get<float>(0) * args.get<t>(1));
        if(args.is_type<float>(1))
            return args.ret(args.get<t>(0) * args.get<float>(1));
        return args.ret(args.get<t>(0) * args.get<t>(1));
    }

    template<typename t> int vec_div(lua_State *state)
    {
        lua_args args(state);
        if(args.is_type<float>(1))
            return args.ret(args.get<t>(0) / args.get<float>(1));
        return args.ret(args.get<t>(0) / args.get<t>(1));
    }

    int vec2_set(lua_State *state) { lua_args args(state); return args.ret(args.get<vec2>(0).set(args.get<float>(1),args.get<float>(2))); }
    int vec3_set(lua_State *state) { lua_args args(state); return args.ret(args.get<vec3>(0).set(args.get<float>(1),args.get<float>(2),args.get<float>(3))); }
    int vec4_set(lua_State *state)
    {
        lua_args args(state);
        vec3 *v1=args.get_p<vec3>(1);
        if(v1)
            return args.ret(args.get<vec4>(0).set(args.get<vec3>(1),args.get<float>(2)));
        return args.ret(args.get<vec4>(0).set(args.get<float>(1),args.get<float>(2),args.get<float>(3),args.get<float>(3)));
    }

    template<typename t> int vec_angle(lua_State *state) { lua_args args(state); return args.ret(args.get<t>(0).angle(args.get<t>(1)).get_rad()); }
    int vec2_get_angle(lua_State *state) { lua_args args(state); return args.ret(args.get_p<vec2>(0)->get_angle().get_rad()); }
    int vec2_rotate(lua_State *state) { lua_args args(state); return args.ret(args.get_p<vec2>(0)->rotate(args.get<float>(1))); }
    int vec3_get_pitch(lua_State *state) { lua_args args(state); return args.ret(args.get_p<vec3>(0)->get_pitch().get_rad()); }
    int vec3_get_yaw(lua_State *state) { lua_args args(state); return args.ret(args.get_p<vec3>(0)->get_yaw().get_rad()); }
    template<typename t> int vec_dot(lua_State *state) { lua_args args(state); return args.ret(args.get<t>(0).dot(args.get<t>(1))); }
    int vec3_cross(lua_State *state) { lua_args args(state); return args.ret(vec3::cross(args.get<vec3>(0),args.get<vec3>(1))); }
    template<typename t> int vec_len(lua_State *state) { lua_args args(state); return args.ret(args.get<t>(0).length()); }
    template<typename t> int vec_len_sq(lua_State *state) { lua_args args(state); return args.ret(args.get<t>(0).length_sq()); }
    template<typename t> int vec_norm(lua_State *state) { lua_args args(state); return args.ret(args.get_p<t>(0)->normalize()); }
    template<typename t> int vec_lerp(lua_State *state) { lua_args args(state); return args.ret(t::lerp(args.get<t>(0),args.get<t>(1),args.get<float>(2))); }
    template<typename t> int vec_abs(lua_State *state) { lua_args args(state); return args.ret(args.get_p<t>(0)->abs()); }
    template<typename t> int vec_refl(lua_State *state) { lua_args args(state); return args.ret(t::reflect(args.get<t>(0),args.get<t>(1))); }
    template<int x,int y> int vec2_const(lua_State *state) { static vec2 v((float)x,(float)y); return lua_args(state).ret(v); }
    template<int x,int y,int z> int vec3_const(lua_State *state) { static vec3 v((float)x,(float)y,(float)z); return lua_args(state).ret(v); }
    template<int x,int y,int z,int w> int vec4_const(lua_State *state) { static vec4 v((float)x,(float)y,(float)z,(float)w); return lua_args(state).ret(v); }

    int quat_new(lua_State *state)
    {
        lua_remove(state,1);
        lua_args args(state);

        vec3 *v0=args.get_p<vec3>(0);
        if(v0)
        {
            if(args.is_type<float>(1)) //axis,angle
                return args.ret(quat(*v0,args.get<float>(1)));
            //from,to
            return args.ret(quat(*v0,args.get<vec3>(1)));
        }

        if (args.count==3) //pyr
            return args.ret(quat(args.get<float>(0),args.get<float>(1),args.get<float>(2)));

        return args.ret(quat(args.get<float>(0),args.get<float>(1),args.get<float>(2),args.get<float>(3)));
    }

    int quat_mul(lua_State *state)
    {
        lua_args args(state);
        return args.ret(args.get<quat>(0) * args.get<quat>(1));
    }

    int quat_invert(lua_State *state) { lua_args args(state); return args.ret(args.get_p<quat>(0)->invert()); }
    int quat_rotate(lua_State *state) { lua_args args(state); return args.ret(args.get<quat>(0).rotate(args.get<vec3>(1))); }
    int quat_rotate_inv(lua_State *state) { lua_args args(state); return args.ret(args.get<quat>(0).rotate_inv(args.get<vec3>(1))); }
    int quat_get_euler(lua_State *state) { lua_args args(state); return args.ret(args.get_p<quat>(0)->get_euler()); }
    int quat_slerp(lua_State *state) { lua_args args(state); return args.ret(quat::slerp(args.get<quat>(0),args.get<quat>(1),args.get<float>(2))); }
    int quat_nlerp(lua_State *state) { lua_args args(state); return args.ret(quat::nlerp(args.get<quat>(0),args.get<quat>(1),args.get<float>(2))); }

    int mat4_new(lua_State *state)
    {
        lua_remove(state,1);
        lua_args args(state);

        quat *q0=args.get_p<quat>(0);
        if(q0)
            return args.ret(mat4(*q0));

        mat4 result;
        if(args.count==17 && args.get<bool>(16)) //transposed
        {
            for(int i=0;i<4;++i) for(int j=0;j<4;++j)
                result[i][j]=args.get<float>(j*4+i);
        }
        else if(args.count>=16)
        {
            for(int i=0;i<4;++i) for(int j=0;j<4;++j)
                result[i][j]=args.get<float>(i*4+j);
        }
        return args.ret(result);
    }

    int mat_mul(lua_State *state)
    {
        lua_args args(state);
        if(args.is_type<vec3>(0))
            return args.ret(args.get<vec3>(0) * args.get<mat4>(1));
        if(args.is_type<vec3>(1))
            return args.ret(args.get<mat4>(0) * args.get<vec3>(1));
        if(args.is_type<vec4>(0))
            return args.ret(args.get<vec4>(0) * args.get<mat4>(1));
        if(args.is_type<vec4>(1))
            return args.ret(args.get<mat4>(0) * args.get<vec4>(1));
        return args.ret(args.get<mat4>(0) * args.get<mat4>(1));
    }

    int mat_identity(lua_State *state) { lua_args args(state); return args.ret(args.get_p<mat4>(0)->identity()); }

    int mat_translate(lua_State *state)
    {
        lua_args args(state);
        vec3 *v1=args.get_p<vec3>(1);
        if(v1)
            return args.ret(args.get_p<mat4>(0)->translate(*v1));
        return args.ret(args.get_p<mat4>(0)->translate(args.get<float>(1),args.get<float>(2),args.get<float>(3)));
    }

    int mat_rotate(lua_State *state)
    {
        lua_args args(state);
        quat *q1=args.get_p<quat>(1);
        if(q1)
            return args.ret(args.get_p<mat4>(0)->rotate(*q1));
        vec3 *v2=args.get_p<vec3>(2);
        if(v2)
            return args.ret(args.get_p<mat4>(0)->rotate(args.get<float>(1),*v2));
        return args.ret(args.get_p<mat4>(0)->rotate(args.get<float>(1),args.get<float>(2),args.get<float>(3),args.get<float>(4)));
    }

    int mat_scale(lua_State *state)
    {
        lua_args args(state);
        vec3 *v1=args.get_p<vec3>(1);
        if(v1)
            return args.ret(args.get_p<mat4>(0)->scale(*v1));
        if(args.count==1)
            return args.ret(args.get_p<mat4>(0)->scale(args.get<float>(1)));
        return args.ret(args.get_p<mat4>(0)->scale(args.get<float>(1),args.get<float>(2),args.get<float>(3)));
    }

    int mat_persp(lua_State *state)
    {
        lua_args args(state);
        return args.ret(args.get_p<mat4>(0)->perspective(args.get<float>(1),args.get<float>(2),args.get<float>(3),args.get<float>(4)));
    }

    int mat_frust(lua_State *state)
    {
        lua_args args(state);
        return args.ret(args.get_p<mat4>(0)->frustrum(args.get<float>(1),args.get<float>(2),args.get<float>(3),
                                                      args.get<float>(4),args.get<float>(5),args.get<float>(6)));
    }

    int mat_ortho(lua_State *state)
    {
        lua_args args(state);
        return args.ret(args.get_p<mat4>(0)->frustrum(args.get<float>(1),args.get<float>(2),args.get<float>(3),
                                                      args.get<float>(4),args.get<float>(5),args.get<float>(6)));
    }

    int mat_invert(lua_State *state) { lua_args args(state); return args.ret(args.get_p<mat4>(0)->invert()); }
    int mat_transpose(lua_State *state) { lua_args args(state); return args.ret(args.get_p<mat4>(0)->transpose()); }
    int mat_get_rot(lua_State *state) { lua_args args(state); return args.ret(args.get_p<mat4>(0)->get_rot()); }
    int mat_get_pos(lua_State *state) { lua_args args(state); return args.ret(args.get_p<mat4>(0)->get_pos()); }

    int lua_lerp(lua_State *state) { lua_args args(state); return args.ret(lerp(args.get<float>(0),args.get<float>(1),args.get<float>(2))); }
    int lua_clamp(lua_State *state) { lua_args args(state); return args.ret(clamp(args.get<float>(0),args.get<float>(1),args.get<float>(2))); }
}

void bind_to_lua(lua_State *state)
{
    static luaL_Reg vec2_f[]=
    {
        {"__unm",vec_unm<vec2>},
        {"__add",vec_add<vec2>},
        {"__sub",vec_sub<vec2>},
        {"__mul",vec_mul<vec2>},
        {"__div",vec_div<vec2>},
        {"_set",vec2_set},
        {"dot",vec_dot<vec2>},
        {"length",vec_len<vec2>},
        {"_length",vec_len<vec2>},
        {"length_sq",vec_len_sq<vec2>},
        {"_length_sq",vec_len_sq<vec2>},
        {"normalize",vec_norm<vec2>},
        {"_normalize",vec_norm<vec2>},
        {"angle",vec_angle<vec2>},
        {"_angle",vec_angle<vec2>},
        {"_get_angle",vec2_get_angle},
        {"rotate",vec2_rotate},
        {"_rotate",vec2_rotate},
        {"lerp",vec_lerp<vec2>},
        {"abs",vec_abs<vec2>},
        {"_abs",vec_abs<vec2>},
        {"reflect",vec_refl<vec2>},
        {"_reflect",vec_refl<vec2>},
        {"right",vec2_const<1,0>},
        {"up",vec2_const<0,1>},
        {"zero",vec2_const<0,0>},
        {"one",vec2_const<1,1>},
        {NULL,NULL}
    };
    table<vec2>::bind(state,"vec2","xy",vec2_f,vec_new<vec2>);

    static luaL_Reg vec3_f[]=
    {
        {"__unm",vec_unm<vec3>},
        {"__add",vec_add<vec3>},
        {"__sub",vec_sub<vec3>},
        {"__mul",vec_mul<vec3>},
        {"__div",vec_div<vec3>},
        {"_set",vec3_set},
        {"dot",vec_dot<vec3>},
        {"cross",vec3_cross},
        {"length",vec_len<vec3>},
        {"_length",vec_len<vec3>},
        {"length_sq",vec_len_sq<vec3>},
        {"_length_sq",vec_len_sq<vec3>},
        {"normalize",vec_norm<vec3>},
        {"_normalize",vec_norm<vec3>},
        {"angle",vec_angle<vec3>},
        {"_angle",vec_angle<vec3>},
        {"_get_pitch",vec3_get_pitch},
        {"_get_yaw",vec3_get_yaw},
        {"lerp",vec_lerp<vec3>},
        {"abs",vec_abs<vec3>},
        {"_abs",vec_abs<vec3>},
        {"reflect",vec_refl<vec3>},
        {"_reflect",vec_refl<vec3>},
        {"forward",vec3_const<0,0,1>},
        {"right",vec3_const<1,0,0>},
        {"up",vec3_const<0,1,0>},
        {"zero",vec3_const<0,0,0>},
        {"one",vec3_const<1,1,1>},
        {NULL,NULL}
    };
    table<vec3>::bind(state,"vec3","xyz",vec3_f,vec_new<vec3>);

    static luaL_Reg vec4_f[]=
    {
        {"__unm",vec_unm<vec4>},
        {"__add",vec_add<vec4>},
        {"__sub",vec_sub<vec4>},
        {"__mul",vec_mul<vec4>},
        {"__div",vec_div<vec4>},
        {"_set",vec4_set},
        {"dot",vec_dot<vec4>},
        {"length",vec_len<vec4>},
        {"_length",vec_len<vec4>},
        {"length_sq",vec_len_sq<vec4>},
        {"_length_sq",vec_len_sq<vec4>},
        {"normalize",vec_norm<vec4>},
        {"_normalize",vec_norm<vec4>},
        {"lerp",vec_lerp<vec4>},
        {"abs",vec_abs<vec4>},
        {"_abs",vec_abs<vec4>},
        {"zero",vec4_const<0,0,0,0>},
        {"one",vec4_const<1,1,1,1>},
        {NULL,NULL}
    };
    table<vec4>::bind(state,"vec4","xyzw",vec4_f,vec_new<vec4>);

    static luaL_Reg quat_f[]=
    {
        {"__unm",vec_unm<quat>},
        {"__mul",quat_mul},
        {"invert",quat_invert},
        {"_invert",quat_invert},
        {"normalize",vec_norm<quat>},
        {"_normalize",vec_norm<quat>},
        {"_rotate",quat_rotate},
        {"_rotate_inv",quat_rotate_inv},
        {"_get_euler",quat_get_euler},
        {"slerp",quat_slerp},
        {"nlerp",quat_nlerp},
        {NULL,NULL}
    };
    table<quat>::bind(state,"quat","xyzw",quat_f,quat_new);

    static luaL_Reg mat4_f[]=
    {
        {"__mul",mat_mul},
        {"_identity",mat_identity},
        {"_translate",mat_translate},
        {"_rotate",mat_rotate},
        {"_scale",mat_scale},
        {"_perspective",mat_persp},
        {"_frustrum",mat_frust},
        {"_ortho",mat_ortho},
        {"_invert",mat_invert},
        {"_transpose",mat_transpose},
        {"_get_pos",mat_get_pos},
        {"_get_rot",mat_get_rot},
        {NULL,NULL}
    };
    table<mat4>::bind(state,"mat4",0,mat4_f,mat4_new);

    static luaL_Reg mathtlib[]=
    {
        {"lerp",lua_lerp},
        {"clamp",lua_clamp},
        {NULL,NULL}
    };
    luaL_openlibs(state);
    lua_getglobal(state,"_G");
    luaL_setfuncs(state,mathtlib,0);
}

const vec2 *lua_testvec2(lua_State *state,int idx) { return table<vec2>::test(state,idx); }
const vec3 *lua_testvec3(lua_State *state,int idx) { return table<vec3>::test(state,idx); }
const vec4 *lua_testvec4(lua_State *state,int idx) { return table<vec4>::test(state,idx); }
const quat *lua_testquat(lua_State *state,int idx) { return table<quat>::test(state,idx); }
const mat4 *lua_testmat4(lua_State *state,int idx) { return table<mat4>::test(state,idx); }

void lua_pushvec2(lua_State *state,const vec2 &v) { table<vec2>::push(state)=v; }
void lua_pushvec3(lua_State *state,const vec3 &v) { table<vec3>::push(state)=v; }
void lua_pushvec4(lua_State *state,const vec4 &v) { table<vec4>::push(state)=v; }
void lua_pushquat(lua_State *state,const quat &q) { table<quat>::push(state)=q; }
void lua_pushmat4(lua_State *state,const mat4 &m) { table<mat4>::push(state)=m; }

}

namespace nya_log
{

namespace
{
    std::string log_pre;
    log_base *lua_log=0;
}

static int lua_print(lua_State* state)
{
    log_base &l=(lua_log?*lua_log:log());
    if(!log_pre.empty())
        l<<log_pre;
    for(int i=1;i<=lua_gettop(state);++i)
    {
        size_t *len=0;
        l<<luaL_tolstring(state,i,len);
        lua_pop(state,1);
    }
    l<<"\n";
    return 0;
}

void set_lua_log(lua_State *state,log_base *l,const char *prefix)
{
    lua_log=l;
    log_pre=prefix?prefix:"";
    static luaL_Reg printlib[]={{"print",lua_print},{NULL,NULL}};
    luaL_openlibs(state);
    lua_getglobal(state,"_G");
    luaL_setfuncs(state,printlib,0);
}

}
