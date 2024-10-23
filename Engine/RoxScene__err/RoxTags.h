// nya-engine (C) nyan.developer@gmail.com released under the MIT license (see LICENSE)

#pragma once

#include <vector>
#include <cstring>
#include <string>

namespace RoxScene
{

    class RTags
    {
    public:
        int getCount() const { return static_cast<int>(mInds.size()); }

        const char* getTag(int idx) const
        {
            if (idx < 0 || idx >= getCount())
                return nullptr;

            return &mBuf[mInds[idx]];
        }

        bool hasTag(const char* tag) const
        {
            if (!tag)
                return false;

            for (int i = 0; i < getCount(); ++i)
            {
                if (std::strcmp(&mBuf[mInds[i]], tag) == 0)
                    return true;
            }

            return false;
        }

        void addTag(const char* tag)
        {
            if (!tag || !*tag || hasTag(tag))
                return;

            mInds.emplace_back(static_cast<unsigned short>(mBuf.size()));
            const char* c = tag;
            do
            {
                mBuf.emplace_back(*c);
            } while (*c++);
        }

        void add(const RTags& t)
        {
            for (int i = 0; i < t.getCount(); ++i)
                addTag(t.getTag(i));
        }

    public:
        RTags() {}

        RTags(const char* str, char delimiter = ',')
        {
            if (!str || !*str)
                return;

            mInds.emplace_back(0);
            char c;
            unsigned short idx = 0;
            while ((c = *str++))
            {
                ++idx;
                if (c == delimiter)
                {
                    mInds.emplace_back(idx);
                    mBuf.emplace_back('\0');
                }
                else
                    mBuf.emplace_back(c);
            }
            mBuf.emplace_back('\0');
        }

    private:
        std::vector<char> mBuf;
        std::vector<unsigned short> mInds;
    };

} // namespace RoxScene
