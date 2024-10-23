// nya-engine (C) nyan.developer@gmail.com released under the MIT license (see LICENSE)

#include "RoxStringConvert.h"
#include <algorithm>
#include <sstream>
#include <cstring> // Replaced <string.h> with <cstring>

namespace RoxFormats
{

    inline std::string fixString(const std::string& s, const std::string& whitespaces = " \t\r\n")
    {
        std::string ss = s;
        std::transform(ss.begin(), ss.end(), ss.begin(), ::tolower);
        const std::size_t startIdx = ss.find_first_not_of(whitespaces);
        if (startIdx == std::string::npos)
            return std::string("");

        const std::size_t endIdx = ss.find_last_not_of(whitespaces) + 1;
        return ss.substr(startIdx, endIdx - startIdx);
    }

    bool boolFromString(const char* s)
    {
        if (!s)
            return false;

        return std::strchr("YyTt1", s[0]) != nullptr;
    }

    RoxMath::Vector4 vec4FromString(const char* s)
    {
        RoxMath::Vector4 v;
        std::string ss = s ? s : "";
        std::replace(ss.begin(), ss.end(), ',', ' ');
        std::istringstream iss(ss);
        if (iss >> v.x)
            if (iss >> v.y)
                if (iss >> v.z)
                    iss >> v.w;
        return v;
    }

    RoxRender::Blend::MODE blendModeFromString(const char* s)
    {
        if (!s)
            return RoxRender::Blend::ONE;

        const std::string ss = fixString(s);
        if (ss == "SRC_ALPHA") return RoxRender::Blend::SRC_ALPHA;
        if (ss == "INV_SRC_ALPHA") return RoxRender::Blend::INV_SRC_ALPHA;
        if (ss == "SRC_COLOR") return RoxRender::Blend::SRC_COLOR;
        if (ss == "INV_SRC_COLOR") return RoxRender::Blend::INV_SRC_COLOR;
        if (ss == "DST_COLOR") return RoxRender::Blend::DST_COLOR;
        if (ss == "INV_DST_COLOR") return RoxRender::Blend::INV_DST_COLOR;
        if (ss == "DST_ALPHA") return RoxRender::Blend::DST_ALPHA;
        if (ss == "INV_DST_ALPHA") return RoxRender::Blend::INV_DST_ALPHA;
        if (ss == "ZERO") return RoxRender::Blend::ZERO;
        if (ss == "ONE") return RoxRender::Blend::ONE;

        return RoxRender::Blend::ONE;
    }

    bool blendModeFromString(const char* s, RoxRender::Blend::MODE& srcOut, RoxRender::Blend::MODE& dstOut)
    {
        if (!s)
            return false;

        std::string str = s;

        const std::size_t divIdx = str.find(':');
        if (divIdx == std::string::npos)
        {
            srcOut = RoxRender::Blend::ONE;
            dstOut = RoxRender::Blend::ZERO;
            return false;
        }

        dstOut = blendModeFromString(str.substr(divIdx + 1).c_str());
        str.resize(divIdx);
        srcOut = blendModeFromString(str.c_str());
        return true;
    }

    bool cullFaceFromString(const char* s, RoxRender::CullFace::ORDER& orderOut)
    {
        if (!s)
            return false;

        const std::string ss = fixString(s);
        if (ss == "cw")
        {
            orderOut = RoxRender::CullFace::CW;
            return true;
        }

        if (ss == "ccw")
        {
            orderOut = RoxRender::CullFace::CCW;
            return true;
        }

        orderOut = RoxRender::CullFace::CCW;
        return false;
    }

} // namespace RoxFormats
