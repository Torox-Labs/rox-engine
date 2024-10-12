//nya-engine (C) nyan.developer@gmail.com released under the MIT license (see LICENSE)

#include "math_expr_parser.h"
#include "math/scalar.h"
#include "math/constants.h"
#include <sstream>
#include <stack>
#include <time.h>
#include <stdlib.h>
#include <string.h>

namespace nya_formats
{

enum op_type
{
    op_sub,
    op_sub_scalar,
    op_sub_vec2,
    op_sub_vec3,
    op_sub_vec4,

    op_add,
    op_add_scalar,
    op_add_vec2,
    op_add_vec3,
    op_add_vec4,

    op_mul,
    op_mul_scalar,
    op_mul_vec2_scalar,
    op_mul_vec3_scalar,
    op_mul_vec4_scalar,

    op_div,
    op_neg,
    op_pow,
    op_less,
    op_more,
    op_less_eq,
    op_more_eq,
    op_func,
    op_user_func
};

enum read_type
{
    read_const=100,
    read_var
};

enum brace
{
    brace_left=200,
    brace_right
};

enum func_type
{
    func_sin=100,
    func_cos,
    func_tan,
    func_atan2,
    func_sqrt,
    func_abs,
    func_rand,
    func_rand2,
    func_min,
    func_max,
    func_floor,
    func_ceil,
    func_fract,
    func_mod,
    func_clamp,
    func_lerp,

    func_len,
    func_len2,
    func_len3,
    func_len4,

    func_norm,
    func_norm2,
    func_norm3,
    func_norm4,

    func_vec2,
    func_vec3,
    func_vec4
};

union op
{
    int i;
    float f;
    brace b;
    op_type o;
    read_type r;
    func_type fnc;
};

struct op_struct
{
    op o,a;

    inline void copy(std::vector<int> &to) const
    {
        to.push_back(o.i);
        if(is_read() || is_function())
            to.push_back(a.i);
    }

    int precedence() const
    {
        switch(o.o)
        {
            case op_less: case op_more: case op_less_eq: case op_more_eq: return 1;
            case op_sub: case op_add: return 2;
            case op_mul: case op_div: return 3;
            case op_pow: return 4;
            case op_neg: return 5;
            case op_func: case op_user_func: return 6;
            default: break;
        }

        return 0;
    }

    bool is_read() const { return o.r==read_const || o.r==read_var; }
    bool is_function() const { return o.o==op_func || o.o==op_user_func; }

    op_struct(op_type ot) { o.o=ot; }
    op_struct(brace b) { o.b=b; }
    op_struct(read_type r) { o.r=r; }
};

inline bool infix_to_rpn(const std::vector<op_struct> &from,std::vector<int> &to)
{
    std::stack<op_struct> stc;
    for(size_t i=0;i<from.size();i++)
    {
        const op_struct &os=from[i];

        if(os.o.b==brace_left)
        {
            stc.push(os);
            continue;
        }

        if(os.o.b==brace_right)
        {
            if(stc.empty())
                return false;

            while(stc.top().o.b!=brace_left)
            {
                stc.top().copy(to);
                stc.pop();

                if(stc.empty())
                    return false;
            }
            stc.pop();
            continue;
        }

        if(!os.precedence())
        {
            os.copy(to);
            continue;
        }

        if(stc.empty() || os.precedence()>stc.top().precedence())
        {
            stc.push(os);
            continue;
        }

        if(os.precedence()==stc.top().precedence() && os.precedence()==4)
        {
            stc.push(os);
            continue;
        }

        stc.top().copy(to);
        stc.pop();
        stc.push(os);
    }

    while(!stc.empty())
    {
        stc.top().copy(to);
        stc.pop();
    }

    return true;
}

inline size_t check_func_var_const(const char *name,bool &is_func,bool &is_var,bool &is_const)
{
    is_func=is_var=is_const=false;
    if(!name)
        return false;

    size_t i=0;

    if(isdigit(*name))
    {
        is_const=true;
        while(*name)
        {
            if(!isdigit(*name) && *name!='.')
                return i;
            ++name,++i;
        }
        return i;
    }

    if(!isalpha(*name) && !strchr("_.", *name))
        return 0;

    while(*name)
    {
        if(*name=='(')
        {
            is_func=true;
            return i;
        }

        if(!isalpha(*name) && !strchr("_.", *name) && !isdigit(*name))
        {
            is_var=true;
            return i;
        }

        ++name,++i;
    }

    is_var=true;
    return i;
}

bool math_expr_parser::parse(const char *expr)
{
    set_constant("pi",nya_math::constants::pi);

    m_ops.clear();
    m_ops_count=0;
    m_stack.set_constant(0.0f);
    if(!expr)
        return false;

    struct init_rand { init_rand() { srand((unsigned int)time(NULL)),rand(); } } static once;

    std::vector<op_struct> ops;
    for(const char *c=expr;*c;++c)
    {
        if(*c<=' ')
            continue;

        switch(*c)
        {
            case '+': ops.push_back(op_struct(op_add)); continue;
            case '-': ops.push_back(op_struct(op_sub)); continue;
            case '*': ops.push_back(op_struct(op_mul)); continue;
            case '/': ops.push_back(op_struct(op_div)); continue;
            case '^': ops.push_back(op_struct(op_pow)); continue;

            case '<':
                if(*(c+1)=='=')
                    ops.push_back(op_struct(op_less_eq)),++c;
                else
                    ops.push_back(op_struct(op_less));
                continue;

            case '>':
                if(*(c+1)=='=')
                    ops.push_back(op_struct(op_more_eq)),++c;
                else
                    ops.push_back(op_struct(op_more));
                continue;

            case '(': ops.push_back(op_struct(brace_left));
                      ops.push_back(op_struct(brace_left)); //ToDo: dublicate for function calls only
                continue;

            case ')': ops.push_back(op_struct(brace_right));
                      ops.push_back(op_struct(brace_right)); //ToDo
                continue;

            case ',': ops.push_back(op_struct(brace_right));
                      ops.push_back(op_struct(brace_left));
                continue;
        }

        bool is_func,is_var,is_const;
        const size_t s=check_func_var_const(c,is_func,is_var,is_const);

        if(is_func)
        {
            op_struct os(op_func);
            const std::string name(c,s);
            if(name=="sin") os.a.fnc=func_sin;
            else if(name=="cos") os.a.fnc=func_cos;
            else if(name=="tan") os.a.fnc=func_tan;
            else if(name=="atan2") os.a.fnc=func_atan2;
            else if(name=="sqrt") os.a.fnc=func_sqrt;
            else if(name=="abs") os.a.fnc=func_abs;
            else if(name=="rand") os.a.fnc=func_rand;
            else if(name=="rand2") os.a.fnc=func_rand2;
            else if(name=="min") os.a.fnc=func_min;
            else if(name=="max") os.a.fnc=func_max;
            else if(name=="floor") os.a.fnc=func_floor;
            else if(name=="ceil") os.a.fnc=func_ceil;
            else if(name=="fract") os.a.fnc=func_fract;
            else if(name=="mod") os.a.fnc=func_mod;
            else if(name=="clamp") os.a.fnc=func_clamp;
            else if(name=="lerp") os.a.fnc=func_lerp;
            else if(name=="length") os.a.fnc=func_len;
            else if(name=="length2") os.a.fnc=func_len2;
            else if(name=="length3") os.a.fnc=func_len3;
            else if(name=="length4") os.a.fnc=func_len4;
            else if(name=="normalize") os.a.fnc=func_norm;
            else if(name=="normalize2") os.a.fnc=func_norm2;
            else if(name=="normalize3") os.a.fnc=func_norm3;
            else if(name=="normalize4") os.a.fnc=func_norm4;
            else if(name=="vec2") os.a.fnc=func_vec2;
            else if(name=="vec3") os.a.fnc=func_vec3;
            else if(name=="vec4") os.a.fnc=func_vec4;
            else
            {
                os.o.o=op_user_func;
                os.a.i=-1;
                for(int i=0;i<(int)m_functions.size();++i)
                {
                    if(m_functions[i].name==name)
                        os.a.i=i;
                }

                if(os.a.i<0)
                    return false;
            }

            ops.push_back(os);
            c+=s-1;
            continue;
        }

        if(is_var)
        {
            const std::string str(c,s);
            c+=s-1;

            bool is_constant=false;
            for(int i=0;i<(int)m_constants.size();++i)
            {
                if(str!=m_constants[i].first)
                    continue;

                op_struct os(read_const);
                os.a.f=m_constants[i].second;
                ops.push_back(os);
                is_constant=true;
                break;
            }

            if(!is_constant)
            {
                op_struct os(read_var);
                os.a.i=add_var(str.c_str());
                ops.push_back(os);
            }

            continue;
        }

        if(is_const)
        {
            const std::string str(c,s);
            c+=s-1;
            op_struct os(read_const);
            std::istringstream iss(str);
            if(!(iss>>os.a.f))
                os.a.f=0.0f;
            ops.push_back(os);
            continue;
        }

        return false;
    }

    //ToDo
    for(size_t i=0;i<ops.size();++i)
    {
        if(!ops[i].is_function())
        {
            if(ops[i].o.o==op_sub && i+1<ops.size() &&
               (ops[i+1].is_read() || ops[i+1].o.b==brace_left || ops[i+1].is_function()))
            {
                if(i && (ops[i-1].is_read() || ops[i-1].o.b==brace_right))
                    continue;

                ops[i].o.o=op_neg;
            }
            continue;
        }

        ops.insert(ops.begin()+i,op_struct(brace_left));
        for(size_t j= ++i,need_brace=0;j<ops.size();++j)
        {
            if(ops[j].o.b==brace_left)
            {
                ++need_brace;
                continue;
            }

            if(ops[j].o.b==brace_right && !(--need_brace))
            {
                ops.insert(ops.begin()+j,op_struct(brace_right));
                break;
            }
        }
    }

    if(!infix_to_rpn(ops,m_ops))
    {
        m_ops.clear();
        return false;
    }

    //ToDo: precalculate constant values

    if(m_ops.size()==2 && m_ops[0]==read_const)
    {
        m_stack.set_constant(*(float *)&m_ops[1]);
        m_ops.clear();
        return true;
    }

    m_ops_count=(int)m_ops.size();

    stack_validator v(m_ops);
    calculate(v);
    if(!v.is_valid())
    {
        m_ops.clear();
        m_ops_count=0;
        return false;
    }

    m_stack.set_size(v.get_size());
    return true;
}

int math_expr_parser::add_var(const char *name)
{
    int idx=get_var_idx(name);
    if(idx>=0)
        return idx;

    if(!name)
        return -1;

    idx=(int)m_vars.size();
    m_vars.resize(idx+1);
    m_var_names.resize(m_vars.size());
    m_var_names.back()=name;
    m_vars.back()=0.0f;
    return idx;
}

template<typename t> float math_expr_parser::calculate(t &stack) const
{
    float a,a2;
    nya_math::vec4 v;
    stack.clear();
    const size_t ops_size=m_ops.size();
    for(size_t i=0;i<ops_size;++i)
    {
        switch(m_ops[i])
        {
            case read_const: stack.add(*((float *)&m_ops[++i])); break;
            case read_var: stack.add(m_vars[m_ops[++i]]); break;

            case op_sub_scalar: a=stack.get(); stack.pop(); stack.get()-=a; break;
            case op_sub_vec2: v.xy()=stack.get2(); stack.template pop<2>(); stack.get2()-=v.xy(); break;
            case op_sub_vec3: v.xyz()=stack.get3(); stack.template pop<3>(); stack.get3()-=v.xyz(); break;
            case op_sub_vec4: v=stack.get4(); stack.template pop<4>(); stack.get4()-=v; break;

            case op_add_scalar: a=stack.get(); stack.pop(); stack.get()+=a; break;
            case op_add_vec2: v.xy()=stack.get2(); stack.template pop<2>(); stack.get2()+=v.xy(); break;
            case op_add_vec3: v.xyz()=stack.get3(); stack.template pop<3>(); stack.get3()+=v.xyz(); break;
            case op_add_vec4: v=stack.get4(); stack.template pop<4>(); stack.get4()+=v; break;

            case op_mul_scalar: a=stack.get(); stack.pop(); stack.get()*=a; break;
            case op_mul_vec2_scalar: a=stack.get(); stack.pop(); stack.get2()*=a; break;
            case op_mul_vec3_scalar: a=stack.get(); stack.pop(); stack.get3()*=a; break;
            case op_mul_vec4_scalar: a=stack.get(); stack.pop(); stack.get4()*=a; break;

            case op_div: a=stack.get(); stack.pop(); stack.get()/=a; break;
            case op_less: a=stack.get(); stack.pop(); stack.get()=float(stack.get()<a); break;
            case op_more: a=stack.get(); stack.pop(); stack.get()=float(stack.get()>a); break;
            case op_less_eq: a=stack.get(); stack.pop(); stack.get()=float(stack.get()<=a); break;
            case op_more_eq: a=stack.get(); stack.pop(); stack.get()=float(stack.get()>=a); break;
            case op_pow: a=stack.get(); stack.pop(); stack.get()=powf(stack.get(),a); break;
            case op_neg: stack.get()= -stack.get(); break;

            case op_func:
                switch(m_ops[++i])
                {
                    case func_rand: stack.add(float(rand()/(RAND_MAX + 1.0f))); break;
                    case func_sin: stack.get()=sinf(stack.get()); break;
                    case func_cos: stack.get()=cosf(stack.get()); break;
                    case func_tan: stack.get()=tanf(stack.get()); break;
                    case func_atan2: a=stack.get(); stack.pop(); stack.get()=atan2f(stack.get(),a); break;
                    case func_sqrt: stack.get()=sqrtf(stack.get()); break;
                    case func_abs: stack.get()=fabsf(stack.get()); break;
                    case func_floor: stack.get()=floorf(stack.get()); break;
                    case func_ceil: stack.get()=ceilf(stack.get()); break;
                    case func_fract: stack.get()=modff(stack.get(),&a); break;
                    case func_min: a=stack.get(); stack.pop(); stack.get()=nya_math::min(stack.get(),a); break;
                    case func_max: a=stack.get(); stack.pop(); stack.get()=nya_math::max(stack.get(),a); break;
                    case func_mod: a=stack.get(); stack.pop(); stack.get()=fmodf(stack.get(),a); break;

                    case func_rand2: a=stack.get(); stack.pop();
                                     stack.get()+=float(rand()/(RAND_MAX + 1.0f))*(a-stack.get()); break;

                    case func_clamp: a2=stack.get(); stack.pop(); a=stack.get(); stack.pop();
                                     stack.get()=nya_math::clamp(stack.get(),a,a2); break;

                    case func_lerp: a2=stack.get(); stack.pop(); a=stack.get(); stack.pop();
                        stack.get()=nya_math::lerp(stack.get(),a,a2); break;

                    case func_len2: stack.get2().x=stack.get2().length(); stack.pop(); break;
                    case func_len3: stack.get3().x=stack.get3().length(); stack.template pop<2>(); break;
                    case func_len4: stack.get4().x=stack.get4().length(); stack.template pop<3>(); break;

                    case func_norm2: stack.get2().normalize(); stack.mark_vec2(); break;
                    case func_norm3: stack.get3().normalize(); stack.mark_vec3(); break;
                    case func_norm4: stack.get4().normalize(); stack.mark_vec4(); break;

                    case func_vec2: stack.mark_vec2(); break;
                    case func_vec3: stack.mark_vec3(); break;
                    case func_vec4: stack.mark_vec4(); break;

                    case func_len: case func_norm: stack.validate_op(i); break;
                }
                break;

            case op_user_func: stack.call(m_functions[m_ops[++i]]); stack.mark_scalar(); break;

            case op_sub: case op_add: case op_mul: stack.validate_op(i); break;
        }
    }

    return stack.get();
}

void math_expr_parser::stack::call(const user_function &f)
{
    float ret[32];
    m_pos-=f.args_count-1;
    f.f(&m_buf[m_pos],ret);
    memcpy(&m_buf[m_pos],ret,f.return_count*sizeof(float));
    m_pos+=f.return_count-1;
}

void math_expr_parser::stack_validator::call(const user_function &f)
{
    if(f.args_count>(int)m_buf.size())
    {
        m_valid=false;
        return;
    }

    m_buf.resize(m_buf.size()-f.args_count+f.return_count,1);
    if((int)m_buf.size()>m_size)
        m_size=(int)m_buf.size();
}

void math_expr_parser::stack_validator::validate_op(size_t idx)
{
    if(m_buf.empty())
    {
        m_valid=false;
        return;
    }

    int type2=m_buf.back();
    if(type2>(int)m_buf.size())
    {
        m_valid=false;
        return;
    }

    switch(m_ops[idx])
    {
        case func_len: m_ops[idx]+=type2; m_buf.resize(m_buf.size()-(type2-1)); return;
        case func_norm: m_ops[idx]+=type2; return;
    }

    m_buf.resize(m_buf.size()-type2);

    if(m_buf.empty())
    {
        m_valid=false;
        return;
    }

    int type1=m_buf.back();

    if(type1>(int)m_buf.size())
    {
        m_valid=false;
        return;
    }

    switch(m_ops[idx])
    {
        case op_add:
        case op_sub:
            if(type1==type2)
            {
                m_ops[idx]+=type1;
                return;
            }
            break;

        case op_mul:
            if(type2==1)
            {
                m_ops[idx]+=type1;
                return;
            }
            break;
    }

    m_valid=false;
}

float math_expr_parser::calculate() const { return calculate(m_stack); }

nya_math::vec4 math_expr_parser::calculate_vec4() const
{
    nya_math::vec4 result;
    result.x=calculate(m_stack);
    m_stack.pop();
    if(m_stack.empty())
        return result;

    result.y=m_stack.get();
    m_stack.pop();
    if(m_stack.empty())
    {
        std::swap(result.x,result.y);
        return result;
    }

    result.z=m_stack.get();
    m_stack.pop();
    if(m_stack.empty())
    {
        std::swap(result.x,result.z);
        return result;
    }

    std::swap(result.y,result.z);
    result.w=result.x;
    result.x=m_stack.get();
    return result;
}

void math_expr_parser::set_constant(const char *name,float value)
{
    if(!name)
        return;

    for(int i=0;i<(int)m_constants.size();++i)
    {
        if(m_constants[i].first==name)
        {
            m_constants[i].second=value;
            return;
        }
    }

    m_constants.push_back(std::make_pair(name,value));
}

int math_expr_parser::get_var_idx(const char *name) const
{
    if(!name)
        return -1;

    for(int i=0;i<get_vars_count();++i)
    {
        if(m_var_names[i]==name)
            return i;
    }

    return -1;
}

int math_expr_parser::get_vars_count() const { return (int)m_vars.size(); }

const char *math_expr_parser::get_var_name(int idx) const
{
    if(idx<0 || idx>=get_vars_count())
        return 0;

    return m_var_names[idx].c_str();
}

bool math_expr_parser::set_var(const char *name,float value,bool allow_unfound)
{
    const int idx=get_var_idx(name);
    if(idx>=0)
    {
        m_vars[idx]=value;
        return true;
    }

    if(!allow_unfound || !name)
        return false;

    m_vars.resize(m_vars.size()+1);
    m_var_names.resize(m_vars.size());
    m_var_names.back()=name;
    m_vars.back()=value;
    return true;
}

bool math_expr_parser::set_var(int idx,float value)
{
    if(idx<0 || idx>=get_vars_count())
        return false;

    m_vars[idx]=value;
    return true;
}

const float *math_expr_parser::get_vars() const { return m_vars.empty()?0:&m_vars[0]; }
float *math_expr_parser::get_vars() { return m_vars.empty()?0:&m_vars[0]; }

void math_expr_parser::set_function(const char *name,int args_count,int return_count,function f)
{
    if(!name || !f || return_count<1 || return_count>32)
        return;

    for(size_t i=0;i<m_functions.size();++i)
    {
        if(m_functions[i].name==name)
        {
            m_functions[i].args_count=args_count;
            m_functions[i].f=f;
            return;
        }
    }

    m_functions.resize(m_functions.size()+1);
    m_functions.back().name.assign(name);
    m_functions.back().args_count=args_count;
    m_functions.back().return_count=return_count;
    m_functions.back().f=f;
}

}
