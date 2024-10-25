// nya-engine (C) nyan.developer@gmail.com released under the MIT license (see LICENSE)

#include "RoxMathExprParser.h"
#include "RoxMath/RoxScalar.h"
#include "RoxMath/RoxConstants.h"

#include <sstream>
#include <stack>
#include <ctime>
#include <cstdlib>
#include <cstring>

namespace RoxFormats
{

    enum OpType
    {
        OP_SUB,
        OP_SUB_SCALAR,
        OP_SUB_VEC2,
        OP_SUB_VEC3,
        OP_SUB_VEC4,

        OP_ADD,
        OP_ADD_SCALAR,
        OP_ADD_VEC2,
        OP_ADD_VEC3,
        OP_ADD_VEC4,

        OP_MUL,
        OP_MUL_SCALAR,
        OP_MUL_VEC2_SCALAR,
        OP_MUL_VEC3_SCALAR,
        OP_MUL_VEC4_SCALAR,

        OP_DIV,
        OP_NEG,
        OP_POW,
        OP_LESS,
        OP_MORE,
        OP_LESS_EQ,
        OP_MORE_EQ,
        OP_FUNC,
        OP_USER_FUNC
    };

    enum ReadType
    {
        READ_CONST = 100,
        READ_VAR
    };

    enum Brace
    {
        BRACE_LEFT = 200,
        BRACE_RIGHT
    };

    enum FuncType
    {
        FUNC_SIN = 100,
        FUNC_COS,
        FUNC_TAN,
        FUNC_ATAN2,
        FUNC_SQRT,
        FUNC_ABS,
        FUNC_RAND,
        FUNC_RAND2,
        FUNC_MIN,
        FUNC_MAX,
        FUNC_FLOOR,
        FUNC_CEIL,
        FUNC_FRACT,
        FUNC_MOD,
        FUNC_CLAMP,
        FUNC_LERP,

        FUNC_LEN,
        FUNC_LEN2,
        FUNC_LEN3,
        FUNC_LEN4,

        FUNC_NORM,
        FUNC_NORM2,
        FUNC_NORM3,
        FUNC_NORM4,

        FUNC_VEC2,
        FUNC_VEC3,
        FUNC_VEC4
    };

    union OpUnion
    {
        int i;
        float f;
        Brace b;
        OpType o;
        ReadType r;
        FuncType fnc;
    };

    struct OpStruct
    {
        OpUnion o;
        OpUnion a;

        inline void copy(std::vector<int>& to) const
        {
            to.push_back(o.i);
            if (isRead() || isFunction())
                to.push_back(a.i);
        }

        int precedence() const
        {
            switch (o.o)
            {
            case OP_LESS:
            case OP_MORE:
            case OP_LESS_EQ:
            case OP_MORE_EQ: return 1;
            case OP_SUB:
            case OP_ADD: return 2;
            case OP_MUL:
            case OP_DIV: return 3;
            case OP_POW: return 4;
            case OP_NEG: return 5;
            case OP_FUNC:
            case OP_USER_FUNC: return 6;
            default: break;
            }

            return 0;
        }

        bool isRead() const { return o.r == READ_CONST || o.r == READ_VAR; }
        bool isFunction() const { return o.o == OP_FUNC || o.o == OP_USER_FUNC; }

        OpStruct(OpType ot) { o.o = ot; }
        OpStruct(Brace bType) { o.b = bType; }
        OpStruct(ReadType rType) { o.r = rType; }
    };

    inline bool infixToRpn(const std::vector<OpStruct>& from, std::vector<int>& to)
    {
        std::stack<OpStruct> stc;
        for (size_t i = 0; i < from.size(); i++)
        {
            const OpStruct& os = from[i];

            if (os.o.b == BRACE_LEFT)
            {
                stc.push(os);
                continue;
            }

            if (os.o.b == BRACE_RIGHT)
            {
                if (stc.empty())
                    return false;

                while (stc.top().o.b != BRACE_LEFT)
                {
                    stc.top().copy(to);
                    stc.pop();

                    if (stc.empty())
                        return false;
                }
                stc.pop();
                continue;
            }

            if (!os.precedence())
            {
                os.copy(to);
                continue;
            }

            if (stc.empty() || os.precedence() > stc.top().precedence())
            {
                stc.push(os);
                continue;
            }

            if (os.precedence() == stc.top().precedence() && os.precedence() == 4)
            {
                stc.push(os);
                continue;
            }

            stc.top().copy(to);
            stc.pop();
            stc.push(os);
        }

        while (!stc.empty())
        {
            stc.top().copy(to);
            stc.pop();
        }

        return true;
    }

    inline size_t checkFuncVarConst(const char* name, bool& isFunc, bool& isVar, bool& isConst)
    {
        isFunc = isVar = isConst = false;
        if (!name)
            return 0;

        size_t i = 0;

        if (isdigit(*name))
        {
            isConst = true;
            while (*name)
            {
                if (!isdigit(*name) && *name != '.')
                    return i;
                ++name, ++i;
            }
            return i;
        }

        if (!isalpha(*name) && !strchr("_.", *name))
            return 0;

        while (*name)
        {
            if (*name == '(')
            {
                isFunc = true;
                return i;
            }

            if (!isalpha(*name) && !strchr("_.", *name) && !isdigit(*name))
            {
                isVar = true;
                return i;
            }

            ++name, ++i;
        }

        isVar = true;
        return i;
    }

    bool RoxMathExprParser::parse(const char* expr)
    {
        setConstant("pi", RoxMath::Constants::pi);

        m_ops.clear();
        m_opsCount = 0;
        m_stack.setConstant(0.0f);
        if (!expr)
            return false;

        struct InitRand { InitRand() { srand(static_cast<unsigned int>(time(nullptr))); rand(); } } static once;

        std::vector<OpStruct> ops;
        for (const char* c = expr; *c; ++c)
        {
            if (*c <= ' ')
                continue;

            switch (*c)
            {
            case '+': ops.emplace_back(OP_ADD); continue;
            case '-': ops.emplace_back(OP_SUB); continue;
            case '*': ops.emplace_back(OP_MUL); continue;
            case '/': ops.emplace_back(OP_DIV); continue;
            case '^': ops.emplace_back(OP_POW); continue;

            case '<':
                if (*(c + 1) == '=')
                {
                    ops.emplace_back(OP_LESS_EQ);
                    ++c;
                }
                else
                {
                    ops.emplace_back(OP_LESS);
                }
                continue;

            case '>':
                if (*(c + 1) == '=')
                {
                    ops.emplace_back(OP_MORE_EQ);
                    ++c;
                }
                else
                {
                    ops.emplace_back(OP_MORE);
                }
                continue;

            case '(':
                ops.emplace_back(BRACE_LEFT);
                ops.emplace_back(BRACE_LEFT); // ToDo: duplicate for function calls only
                continue;

            case ')':
                ops.emplace_back(BRACE_RIGHT);
                ops.emplace_back(BRACE_RIGHT); // ToDo
                continue;

            case ',':
                ops.emplace_back(BRACE_RIGHT);
                ops.emplace_back(BRACE_LEFT);
                continue;
            }

            bool isFunc, isVar, isConst;
            const size_t s = checkFuncVarConst(c, isFunc, isVar, isConst);

            if (isFunc)
            {
                OpStruct os(OP_FUNC);
                const std::string name(c, s);
                if (name == "sin") os.a.fnc = FUNC_SIN;
                else if (name == "cos") os.a.fnc = FUNC_COS;
                else if (name == "tan") os.a.fnc = FUNC_TAN;
                else if (name == "atan2") os.a.fnc = FUNC_ATAN2;
                else if (name == "sqrt") os.a.fnc = FUNC_SQRT;
                else if (name == "abs") os.a.fnc = FUNC_ABS;
                else if (name == "rand") os.a.fnc = FUNC_RAND;
                else if (name == "rand2") os.a.fnc = FUNC_RAND2;
                else if (name == "min") os.a.fnc = FUNC_MIN;
                else if (name == "max") os.a.fnc = FUNC_MAX;
                else if (name == "floor") os.a.fnc = FUNC_FLOOR;
                else if (name == "ceil") os.a.fnc = FUNC_CEIL;
                else if (name == "fract") os.a.fnc = FUNC_FRACT;
                else if (name == "mod") os.a.fnc = FUNC_MOD;
                else if (name == "clamp") os.a.fnc = FUNC_CLAMP;
                else if (name == "lerp") os.a.fnc = FUNC_LERP;
                else if (name == "length") os.a.fnc = FUNC_LEN;
                else if (name == "length2") os.a.fnc = FUNC_LEN2;
                else if (name == "length3") os.a.fnc = FUNC_LEN3;
                else if (name == "length4") os.a.fnc = FUNC_LEN4;
                else if (name == "normalize") os.a.fnc = FUNC_NORM;
                else if (name == "normalize2") os.a.fnc = FUNC_NORM2;
                else if (name == "normalize3") os.a.fnc = FUNC_NORM3;
                else if (name == "normalize4") os.a.fnc = FUNC_NORM4;
                else if (name == "vec2") os.a.fnc = FUNC_VEC2;
                else if (name == "vec3") os.a.fnc = FUNC_VEC3;
                else if (name == "vec4") os.a.fnc = FUNC_VEC4;
                else
                {
                    os.o.o = OP_USER_FUNC;
                    os.a.i = -1;
                    for (int i = 0; i < static_cast<int>(m_functions.size()); ++i)
                    {
                        if (m_functions[i].name == name)
                            os.a.i = i;
                    }

                    if (os.a.i < 0)
                        return false;
                }

                ops.emplace_back(os);
                c += s - 1;
                continue;
            }

            if (isVar)
            {
                const std::string str(c, s);
                c += s - 1;

                bool isConstant = false;
                for (int i = 0; i < static_cast<int>(m_constants.size()); ++i)
                {
                    if (str != m_constants[i].first)
                        continue;

                    OpStruct os(READ_CONST);
                    os.a.f = m_constants[i].second;
                    ops.emplace_back(os);
                    isConstant = true;
                    break;
                }

                if (!isConstant)
                {
                    OpStruct os(READ_VAR);
                    os.a.i = addVar(str.c_str());
                    ops.emplace_back(os);
                }

                continue;
            }

            if (isConst)
            {
                const std::string str(c, s);
                c += s - 1;
                OpStruct os(READ_CONST);
                std::istringstream iss(str);
                if (!(iss >> os.a.f))
                    os.a.f = 0.0f;
                ops.emplace_back(os);
                continue;
            }

            return false;
        }

        // ToDo: Handle function calls and brace duplications appropriately
        for (size_t i = 0; i < ops.size(); ++i)
        {
            if (!ops[i].isFunction())
            {
                if (ops[i].o.o == OP_SUB && i + 1 < ops.size() &&
                    (ops[i + 1].isRead() || ops[i + 1].o.b == BRACE_LEFT || ops[i + 1].isFunction()))
                {
                    if (i && (ops[i - 1].isRead() || ops[i - 1].o.b == BRACE_RIGHT))
                        continue;

                    ops[i].o.o = OP_NEG;
                }
                continue;
            }

            ops.insert(ops.begin() + i, OpStruct(BRACE_LEFT));
            for (size_t j = ++i, needBrace = 0; j < ops.size(); ++j)
            {
                if (ops[j].o.b == BRACE_LEFT)
                {
                    ++needBrace;
                    continue;
                }

                if (ops[j].o.b == BRACE_RIGHT && !(--needBrace))
                {
                    ops.insert(ops.begin() + j, OpStruct(BRACE_RIGHT));
                    break;
                }
            }
        }

        if (!infixToRpn(ops, m_ops))
        {
            m_ops.clear();
            return false;
        }

        // ToDo: Precalculate constant values

        if (m_ops.size() == 2 && m_ops[0] == READ_CONST)
        {
            m_stack.setConstant(*reinterpret_cast<const float*>(&m_ops[1]));
            m_ops.clear();
            return true;
        }

        m_opsCount = static_cast<int>(m_ops.size());

        RoxStackValidator v(m_ops);
        calculate(v);
        if (!v.isValid())
        {
            m_ops.clear();
            m_opsCount = 0;
            return false;
        }

        m_stack.setSize(v.getSize());
        return true;
    }

    int RoxMathExprParser::addVar(const char* name)
    {
        int idx = getVarIdx(name);
        if (idx >= 0)
            return idx;

        if (!name)
            return -1;

        idx = static_cast<int>(m_vars.size());
        m_vars.resize(idx + 1);
        m_varNames.resize(m_vars.size());
        m_varNames.back() = name;
        m_vars.back() = 0.0f;
        return idx;
    }

    template<typename T>
    float RoxMathExprParser::calculate(T& stack) const
    {
        float a, a2;
        RoxMath::Vector4 v;
        stack.clear();
        const size_t opsSize = m_ops.size();
        for (size_t i = 0; i < opsSize; ++i)
        {
            switch (m_ops[i])
            {
            case READ_CONST:
                stack.add(*reinterpret_cast<const float*>(&m_ops[++i]));
                break;
            case READ_VAR:
                stack.add(m_vars[m_ops[++i]]);
                break;

            case OP_SUB_SCALAR:
                a = stack.get();
                stack.pop();
                stack.get() -= a;
                break;
            case OP_SUB_VEC2:
                v.xy() = stack.get2();
                stack.template pop<2>();
                stack.get2() -= v.xy();
                break;
            case OP_SUB_VEC3:
                v.xyz() = stack.get3();
                stack.template pop<3>();
                stack.get3() -= v.xyz();
                break;
            case OP_SUB_VEC4:
                v = stack.get4();
                stack.template pop<4>();
                stack.get4() -= v;
                break;

            case OP_ADD_SCALAR:
                a = stack.get();
                stack.pop();
                stack.get() += a;
                break;
            case OP_ADD_VEC2:
                v.xy() = stack.get2();
                stack.template pop<2>();
                stack.get2() += v.xy();
                break;
            case OP_ADD_VEC3:
                v.xyz() = stack.get3();
                stack.template pop<3>();
                stack.get3() += v.xyz();
                break;
            case OP_ADD_VEC4:
                v = stack.get4();
                stack.template pop<4>();
                stack.get4() += v;
                break;

            case OP_MUL_SCALAR:
                a = stack.get();
                stack.pop();
                stack.get() *= a;
                break;
            case OP_MUL_VEC2_SCALAR:
                a = stack.get();
                stack.pop();
                stack.get2() *= a;
                break;
            case OP_MUL_VEC3_SCALAR:
                a = stack.get();
                stack.pop();
                stack.get3() *= a;
                break;
            case OP_MUL_VEC4_SCALAR:
                a = stack.get();
                stack.pop();
                stack.get4() *= a;
                break;

            case OP_DIV:
                a = stack.get();
                stack.pop();
                stack.get() /= a;
                break;
            case OP_LESS:
                a = stack.get();
                stack.pop();
                stack.get() = static_cast<float>(stack.get() < a);
                break;
            case OP_MORE:
                a = stack.get();
                stack.pop();
                stack.get() = static_cast<float>(stack.get() > a);
                break;
            case OP_LESS_EQ:
                a = stack.get();
                stack.pop();
                stack.get() = static_cast<float>(stack.get() <= a);
                break;
            case OP_MORE_EQ:
                a = stack.get();
                stack.pop();
                stack.get() = static_cast<float>(stack.get() >= a);
                break;
            case OP_POW:
                a = stack.get();
                stack.pop();
                stack.get() = powf(stack.get(), a);
                break;
            case OP_NEG:
                stack.get() = -stack.get();
                break;

            case OP_FUNC:
                switch (static_cast<FuncType>(m_ops[++i]))
                {
                case FUNC_RAND:
                    stack.add(static_cast<float>(rand()) / (RAND_MAX + 1.0f));
                    break;
                case FUNC_SIN:
                    stack.get() = sinf(stack.get());
                    break;
                case FUNC_COS:
                    stack.get() = cosf(stack.get());
                    break;
                case FUNC_TAN:
                    stack.get() = tanf(stack.get());
                    break;
                case FUNC_ATAN2:
                    a = stack.get();
                    stack.pop();
                    stack.get() = atan2f(stack.get(), a);
                    break;
                case FUNC_SQRT:
                    stack.get() = sqrtf(stack.get());
                    break;
                case FUNC_ABS:
                    stack.get() = fabsf(stack.get());
                    break;
                case FUNC_FLOOR:
                    stack.get() = floorf(stack.get());
                    break;
                case FUNC_CEIL:
                    stack.get() = ceilf(stack.get());
                    break;
                case FUNC_FRACT:
                    stack.get() = modff(stack.get(), &a);
                    break;
                case FUNC_MIN:
                    a = stack.get();
                    stack.pop();
                    stack.get() = RoxMath::min(stack.get(), a);
                    break;
                case FUNC_MAX:
                    a = stack.get();
                    stack.pop();
                    stack.get() = RoxMath::max(stack.get(), a);
                    break;
                case FUNC_MOD:
                    a = stack.get();
                    stack.pop();
                    stack.get() = fmodf(stack.get(), a);
                    break;

                case FUNC_RAND2:
                    a = stack.get();
                    stack.pop();
                    stack.get() += static_cast<float>(rand()) / (RAND_MAX + 1.0f) * (a - stack.get());
                    break;

                case FUNC_CLAMP:
                    a2 = stack.get();
                    stack.pop();
                    a = stack.get();
                    stack.pop();
                    stack.get() = RoxMath::clamp(stack.get(), a, a2);
                    break;

                case FUNC_LERP:
                    a2 = stack.get();
                    stack.pop();
                    a = stack.get();
                    stack.pop();
                    stack.get() = RoxMath::lerp(stack.get(), a, a2);
                    break;

                case FUNC_LEN2:
                    stack.get2().x = stack.get2().length();
                    stack.pop();
                    break;
                case FUNC_LEN3:
                    stack.get3().x = stack.get3().length();
                    stack.template pop<2>();
                    break;
                case FUNC_LEN4:
                    stack.get4().x = stack.get4().length();
                    stack.template pop<3>();
                    break;

                case FUNC_NORM2:
                    stack.get2().normalize();
                    stack.markVec2();
                    break;
                case FUNC_NORM3:
                    stack.get3().normalize();
                    stack.markVec3();
                    break;
                case FUNC_NORM4:
                    stack.get4().normalize();
                    stack.markVec4();
                    break;

                case FUNC_VEC2:
                    stack.markVec2();
                    break;
                case FUNC_VEC3:
                    stack.markVec3();
                    break;
                case FUNC_VEC4:
                    stack.markVec4();
                    break;

                case FUNC_LEN:
                case FUNC_NORM:
                    stack.validateOp(i);
                    break;
                }
                break;

            case OP_USER_FUNC:
                stack.call(m_functions[m_ops[++i]]);
                stack.markScalar();
                break;

            case OP_SUB:
            case OP_ADD:
            case OP_MUL:
                stack.validateOp(i);
                break;
            }
        }

        return stack.get();
    }

    void RoxMathExprParser::RoxStack::call(const UserFunction& f)
    {
        float ret[32];
        m_pos -= f.argsCount - 1;
        f.f(&m_buf[m_pos], ret);
        std::memcpy(&m_buf[m_pos], ret, f.returnCount * sizeof(float));
        m_pos += f.returnCount - 1;
    }

    void RoxMathExprParser::RoxStackValidator::call(const UserFunction& f)
    {
        if (f.argsCount > static_cast<int>(m_buf.size()))
        {
            m_valid = false;
            return;
        }

        m_buf.resize(m_buf.size() - f.argsCount + f.returnCount, 1);
        if (static_cast<int>(m_buf.size()) > m_size)
            m_size = static_cast<int>(m_buf.size());
    }

    void RoxMathExprParser::RoxStackValidator::validateOp(size_t idx)
    {
        if (m_buf.empty())
        {
            m_valid = false;
            return;
        }

        int type2 = m_buf.back();
        if (type2 > static_cast<int>(m_buf.size()))
        {
            m_valid = false;
            return;
        }

        switch (m_ops[idx])
        {
        case FUNC_LEN:
            m_ops[idx] += type2;
            m_buf.resize(m_buf.size() - (type2 - 1));
            return;
        case FUNC_NORM:
            m_ops[idx] += type2;
            return;
        }

        m_buf.resize(m_buf.size() - type2);

        if (m_buf.empty())
        {
            m_valid = false;
            return;
        }

        int type1 = m_buf.back();

        if (type1 > static_cast<int>(m_buf.size()))
        {
            m_valid = false;
            return;
        }

        switch (m_ops[idx])
        {
        case OP_ADD:
        case OP_SUB:
            if (type1 == type2)
            {
                m_ops[idx] += type1;
                return;
            }
            break;

        case OP_MUL:
            if (type2 == 1)
            {
                m_ops[idx] += type1;
                return;
            }
            break;
        }

        m_valid = false;
    }

    float RoxMathExprParser::calculate() const { return calculate(m_stack); }

    RoxMath::Vector4 RoxMathExprParser::calculateVec4() const
    {
        RoxMath::Vector4 result;
        result.x = calculate(m_stack);
        m_stack.pop();
        if (m_stack.empty())
            return result;

        result.y = m_stack.get();
        m_stack.pop();
        if (m_stack.empty())
        {
            std::swap(result.x, result.y);
            return result;
        }

        result.z = m_stack.get();
        m_stack.pop();
        if (m_stack.empty())
        {
            std::swap(result.x, result.z);
            return result;
        }

        std::swap(result.y, result.z);
        result.w = result.x;
        result.x = m_stack.get();
        return result;
    }

    void RoxMathExprParser::setConstant(const char* name, float value)
    {
        if (!name)
            return;

        for (int i = 0; i < static_cast<int>(m_constants.size()); ++i)
        {
            if (m_constants[i].first == name)
            {
                m_constants[i].second = value;
                return;
            }
        }

        m_constants.emplace_back(std::make_pair(name, value));
    }

    int RoxMathExprParser::getVarIdx(const char* name) const
    {
        if (!name)
            return -1;

        for (int i = 0; i < getVarsCount(); ++i)
        {
            if (m_varNames[i] == name)
                return i;
        }

        return -1;
    }

    int RoxMathExprParser::getVarsCount() const { return static_cast<int>(m_vars.size()); }

    const char* RoxMathExprParser::getVarName(int idx) const
    {
        if (idx < 0 || idx >= getVarsCount())
            return nullptr;

        return m_varNames[idx].c_str();
    }

    bool RoxMathExprParser::setVar(const char* name, float value, bool allowUnfound)
    {
        const int idx = getVarIdx(name);
        if (idx >= 0)
        {
            m_vars[idx] = value;
            return true;
        }

        if (!allowUnfound || !name)
            return false;

        m_vars.emplace_back(value);
        m_varNames.emplace_back(name);
        return true;
    }

    bool RoxMathExprParser::setVar(int idx, float value)
    {
        if (idx < 0 || idx >= getVarsCount())
            return false;

        m_vars[idx] = value;
        return true;
    }

    const float* RoxMathExprParser::getVars() const { return m_vars.empty() ? nullptr : &m_vars[0]; }
    float* RoxMathExprParser::getVars() { return m_vars.empty() ? nullptr : &m_vars[0]; }

    void RoxMathExprParser::setFunction(const char* name, int argsCount, int returnCount, Function f)
    {
        if (!name || !f || returnCount < 1 || returnCount > 32)
            return;

        for (size_t i = 0; i < m_functions.size(); ++i)
        {
            if (m_functions[i].name == name)
            {
                m_functions[i].argsCount = argsCount;
                m_functions[i].f = f;
                return;
            }
        }

        m_functions.emplace_back(UserFunction{ name, static_cast<char>(argsCount), static_cast<char>(returnCount), f });
    }

} // namespace RoxFormats
