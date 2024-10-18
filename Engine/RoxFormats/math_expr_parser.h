//nya-engine (C) nyan.developer@gmail.com released under the MIT license (see LICENSE)

#pragma once

#include <vector>
#include <string>
#include "math/vector.h"

namespace nya_formats
{

class math_expr_parser
{
public:
    typedef void (*function)(float *args,float *return_value);
    void set_function(const char *name,int args_count,int return_count,function f);
    void set_constant(const char *name,float value);

public:
    bool parse(const char *expr);

public:
    int get_var_idx(const char *name) const;
    int get_vars_count() const;
    const char *get_var_name(int idx) const;
    bool set_var(const char *name,float value,bool allow_unfound=true);
    bool set_var(int idx,float value);
    const float *get_vars() const;
    float *get_vars();

public:
    float calculate() const;
    nya_math::vec4 calculate_vec4() const;

public:
    math_expr_parser(): m_ops_count(0) {}
    math_expr_parser(const char *expr) { parse(expr); }

private:
    int add_var(const char *name);
    template<typename t> float calculate(t &stack) const;

private:
    std::vector<std::pair<std::string,float> > m_constants;
    std::vector<float> m_vars;
    std::vector<std::string> m_var_names;
    std::vector<int> m_ops;

    struct user_function { std::string name; char args_count,return_count; function f; };
    std::vector<user_function> m_functions;

    class stack_validator
    {
    public:
        void clear() { m_buf.clear(); }
        void add(float f) { m_buf.push_back(1); if((int)m_buf.size()>m_size) m_size=(int)m_buf.size(); }
        float &get() { return get_n<float>(); }
        nya_math::vec2 &get2() { return get_n<nya_math::vec2>(); }
        nya_math::vec3 &get3() { return get_n<nya_math::vec3>(); }
        nya_math::vec4 &get4() { return get_n<nya_math::vec4>(); }
        template<int count>void pop() { if(m_buf.size()<count) m_valid=false; else m_buf.resize(m_buf.size()-count); }
        void pop() { if(m_buf.empty()) m_valid=false; else m_buf.pop_back(); }        
        void call(const user_function &f);

    public:
        void validate_op(size_t idx);

    public:
        void mark_scalar() { mark_n<1>(); }
        void mark_vec2() { mark_n<2>(); }
        void mark_vec3() { mark_n<3>(); }
        void mark_vec4() { mark_n<4>(); }

    public:
        bool is_valid() { return m_valid; }
        int get_size() { return m_size; }

    public:
        stack_validator(std::vector<int> &ops): m_valid(true),m_ops(ops),m_size(0) {}

    private:
        template<typename t> t &get_n() { static t v; if(m_buf.size()<sizeof(t)/sizeof(float)) m_valid=false; return v; }
        template<int count> void mark_n() { if(m_buf.size()<count) m_valid=false; else for(int i=0;i<count;++i) m_buf[m_buf.size()-i-1]=count; }

    private:
        bool m_valid;
        std::vector<int> m_buf;
        std::vector<int> &m_ops;
        int m_size;
    };

    class stack
    {
    public:
        void clear() { m_pos=0; }
        void add(const float &f) { m_buf[++m_pos]=f; }
        float &get() { return m_buf[m_pos]; }
        nya_math::vec2 &get2() { return get_n<nya_math::vec2>(); }
        nya_math::vec3 &get3() { return get_n<nya_math::vec3>(); }
        nya_math::vec4 &get4() { return get_n<nya_math::vec4>(); }

        template<int count>void pop() { m_pos-=count; }
        void pop() { --m_pos; }
        void call(const user_function &f);
        bool empty() { return m_pos<=0; }

    public:
        //m_buf[0] is reserved for constant state
        void set_size(int size) { m_buf.resize(size+1,0.0f); m_pos=0; }
        void set_constant(float value) { m_buf.resize(1); m_buf[0]=value; m_pos=0; }

    public:
        stack() { set_constant(0.0f); }

    public:
        void validate_op(size_t idx) {}

    public:
        void mark_scalar() {}
        void mark_vec2() {}
        void mark_vec3() {}
        void mark_vec4() {}

    private:
        template<typename t> t &get_n() { return *(t*)&m_buf[m_pos-sizeof(t)/sizeof(float)+1]; }

    private:
        std::vector<float> m_buf;
        int m_pos;
    };

    mutable stack m_stack;
    int m_ops_count;
};

}
