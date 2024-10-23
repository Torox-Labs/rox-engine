// nya-engine (C) nyan.developer@gmail.com released under the MIT license (see LICENSE)

#pragma once

#include "RoxMath/RoxVector.h"
#include <vector>
#include <string>

namespace RoxFormats
{

    class RoxMathExprParser
    {
    public:
        using Function = void (*)(float* args, float* return_value);

        void setFunction(const char* name, int argsCount, int returnCount, Function f);
        void setConstant(const char* name, float value);

    public:
        bool parse(const char* expr);

    public:
        int getVarIdx(const char* name) const;
        int getVarsCount() const;
        const char* getVarName(int idx) const;
        bool setVar(const char* name, float value, bool allowUnfound = true);
        bool setVar(int idx, float value);
        const float* getVars() const;
        float* getVars();

    public:
        float calculate() const;
        RoxMath::Vector4 calculateVec4() const;

    public:
        RoxMathExprParser() : m_opsCount(0) {}
        RoxMathExprParser(const char* expr) { parse(expr); }

    private:
        int addVar(const char* name);
        template<typename T>
        float calculate(T& stack) const;

    private:
        std::vector<std::pair<std::string, float>> m_constants;
        std::vector<float> m_vars;
        std::vector<std::string> m_varNames;
        std::vector<int> m_ops;

        struct UserFunction
        {
            std::string name;
            char argsCount;
            char returnCount;
            Function f;
        };
        std::vector<UserFunction> m_functions;

        class RoxStackValidator
        {
        public:
            void clear() { m_buf.clear(); }
            void add(float f) { m_buf.push_back(1); if (static_cast<int>(m_buf.size()) > m_size) m_size = static_cast<int>(m_buf.size()); }
            float& get() { return getN<float>(); }
            RoxMath::Vector2& get2() { return getN<RoxMath::Vector2>(); }
            RoxMath::Vector3& get3() { return getN<RoxMath::Vector3>(); }
            RoxMath::Vector4& get4() { return getN<RoxMath::Vector4>(); }
            template<int Count> void pop() { if (static_cast<int>(m_buf.size()) < Count) m_valid = false; else m_buf.resize(m_buf.size() - Count); }
            void pop() { if (m_buf.empty()) m_valid = false; else m_buf.pop_back(); }
            void call(const UserFunction& f);

        public:
            void validateOp(size_t idx);

        public:
            void markScalar() { markN<1>(); }
            void markVec2() { markN<2>(); }
            void markVec3() { markN<3>(); }
            void markVec4() { markN<4>(); }

        public:
            bool isValid() const { return m_valid; }
            int getSize() const { return m_size; }

        public:
            RoxStackValidator(std::vector<int>& ops) : m_valid(true), m_ops(ops), m_size(0) {}

        private:
            template<typename T>
            T& getN() { static T v; if (static_cast<int>(m_buf.size()) < sizeof(T) / sizeof(float)) m_valid = false; return v; }
            template<int Count> void markN()
            {
                if (static_cast<int>(m_buf.size()) < Count) { m_valid = false; }
                else { for (int i = 0; i < Count; ++i) m_buf[m_buf.size() - i - 1] = Count; }
            }

        private:
            bool m_valid;
            std::vector<int> m_buf;
            std::vector<int>& m_ops;
            int m_size;
        };

        class RoxStack
        {
        public:
            void clear() { m_pos = 0; }
            void add(const float& f) { m_buf[++m_pos] = f; }
            float& get() { return m_buf[m_pos]; }
            RoxMath::Vector2& get2() { return getN<RoxMath::Vector2>(); }
            RoxMath::Vector3& get3() { return getN<RoxMath::Vector3>(); }
            RoxMath::Vector4& get4() { return getN<RoxMath::Vector4>(); }

            template<int Count> void pop() { m_pos -= Count; }
            void pop() { --m_pos; }
            void call(const UserFunction& f);
            bool empty() const { return m_pos <= 0; }

        public:
            // m_buf[0] is reserved for constant state
            void setSize(int size) { m_buf.resize(size + 1, 0.0f); m_pos = 0; }
            void setConstant(float value) { m_buf.resize(1, value); m_pos = 0; }

        public:
            RoxStack() { setConstant(0.0f); }

        public:
            void validateOp(size_t idx) {}

        public:
            void markScalar() {}
            void markVec2() {}
            void markVec3() {}
            void markVec4() {}

        private:
            template<typename T>
            T& getN() { return *(reinterpret_cast<T*>(&m_buf[m_pos - sizeof(T) / sizeof(float) + 1])); }

        private:
            std::vector<float> m_buf;
            int m_pos;
        };

        mutable RoxStack m_stack;
        int m_opsCount;
    };

} // namespace RoxFormats
