// Updated By the ROX_ENGINE
// Copyright (C) 2024 Torox Project
// Portions Copyright (C) 2013 nyan.developer@gmail.com (nya-engine)
//
// This file was modified by the Torox Project.
// 
// This file incorporates code from the nya-engine project, which is licensed under the MIT License.
// See the LICENSE-MIT file in the root directory for more information.
//
// This file is also part of the Rox-engine, which is licensed under a dual-license system:
// 1. Free Use License (for non-commercial and commercial use under specific conditions)
// 2. Commercial License (for use on proprietary platforms)
// See the LICENSE file in the root directory for the full Rox-engine license terms.

#include "RoxTextParser.h"
#include "RoxLogger/RoxLogger.h"
#include "RoxMemory/RoxInvalidObject.h"
#include "RoxStringConvert.h"

#include <iostream>
#include <sstream>
#include <algorithm>
#include <cstring>

#ifdef _MSC_VER
#define strcasecmp _stricmp
#endif

namespace
{
    const char globalMarker = '@';
    const char* specialChars = "=:";
}

namespace RoxFormats
{

    const char* RTextParser::getSectionType(int idx) const
    {
        if (idx < 0 || idx >= static_cast<int>(mSections.size()))
            return nullptr;

        return mSections[idx].type.c_str();
    }

    bool RTextParser::isSectionType(int idx, const char* type) const
    {
        if (!type || idx < 0 || idx >= static_cast<int>(mSections.size()))
            return false;

        if (type[0] == globalMarker)
            return mSections[idx].type == type;

        return mSections[idx].type.compare(1, mSections[idx].type.size() - 1, type) == 0;
    }

    int RTextParser::getSectionNamesCount(int idx) const
    {
        if (idx < 0 || idx >= static_cast<int>(mSections.size()))
            return 0;

        return static_cast<int>(mSections[idx].names.size());
    }

    const char* RTextParser::getSectionName(int idx, int nameIdx) const
    {
        if (idx < 0 || idx >= static_cast<int>(mSections.size()))
            return nullptr;

        if (nameIdx < 0 || nameIdx >= static_cast<int>(mSections[idx].names.size()))
            return nullptr;

        return mSections[idx].names[nameIdx].c_str();
    }

    const char* RTextParser::getSectionOption(int idx) const
    {
        if (idx < 0 || idx >= static_cast<int>(mSections.size()))
            return nullptr;

        return mSections[idx].option.c_str();
    }

    const char* RTextParser::getSectionValue(int idx) const
    {
        if (idx < 0 || idx >= static_cast<int>(mSections.size()))
            return nullptr;

        return mSections[idx].value.c_str();
    }

    RoxMath::Vector4 RTextParser::getSectionValueVec4(int idx) const
    {
        if (idx < 0 || idx >= static_cast<int>(mSections.size()))
            return RoxMemory::invalidObject<RoxMath::Vector4>();

        return vec4FromString(mSections[idx].value.c_str());
    }

    inline void removeQuotes(std::string& s)
    {
        if (s.empty())
            return;

        if (s.front() == '"' && s.back() == '"')
            s = s.substr(1, s.size() - 2);
    }

    int RTextParser::getSubsectionsCount(int sectionIdx) const
    {
        if (sectionIdx < 0 || sectionIdx >= static_cast<int>(mSections.size()))
            return -1;

        const Section& s = mSections[sectionIdx];
        if (!s.subsectionParsed)
        {
            // Parse subsections
            Line l = Line::first(s.value.c_str(), s.value.size());
            while (l.next())
            {
                const char* text = l.text + l.offset;
                std::size_t off = skipWhitespaces(text, l.size, 0);
                if (off == l.size)
                    continue;

                text += off;
                std::size_t roff = skipWhitespacesBack(text, l.size - off - 1);

                std::size_t eq = std::string::npos;
                for (std::size_t i = 0; i < roff; ++i)
                {
                    if (text[i] == '=')
                    {
                        eq = i;
                        break;
                    }
                }

                if (eq == std::string::npos)
                {
                    s.subsections.emplace_back();
                    Subsection& ss = s.subsections.back();
                    ss.type = std::string(text, roff);
                    removeQuotes(ss.type);
                    continue;
                }

                s.subsections.emplace_back();
                Subsection& ss = s.subsections.back();

                ss.type = std::string(text, skipWhitespacesBack(text, eq - 1));
                removeQuotes(ss.type);
                eq = skipWhitespaces(text, roff, eq + 1);
                ss.value = std::string(text + eq, roff - eq);
                removeQuotes(ss.value);
            }

            s.subsectionParsed = true;
        }

        return static_cast<int>(s.subsections.size());
    }

    const char* RTextParser::getSubsectionType(int sectionIdx, int idx) const
    {
        if (idx < 0 || idx >= getSubsectionsCount(sectionIdx))
            return nullptr;

        return mSections[sectionIdx].subsections[idx].type.c_str();
    }

    const char* RTextParser::getSubsectionValue(int sectionIdx, int idx) const
    {
        if (idx < 0 || idx >= getSubsectionsCount(sectionIdx))
            return nullptr;

        return mSections[sectionIdx].subsections[idx].value.c_str();
    }

    const char* RTextParser::getSubsectionValue(int sectionIdx, const char* type) const
    {
        if (!type)
            return nullptr;

        for (int i = 0; i < getSubsectionsCount(sectionIdx); ++i)
        {
            if (mSections[sectionIdx].subsections[i].type == type)
                return mSections[sectionIdx].subsections[i].value.c_str();
        }

        return nullptr;
    }

    bool RTextParser::getSubsectionValueBool(int sectionIdx, int idx) const
    {
        if (idx < 0 || idx >= getSubsectionsCount(sectionIdx))
            return false;

        return boolFromString(mSections[sectionIdx].subsections[idx].value.c_str());
    }

    int RTextParser::getSubsectionValueInt(int sectionIdx, int idx) const
    {
        if (idx < 0 || idx >= getSubsectionsCount(sectionIdx))
            return 0;

        int ret = 0;
        std::istringstream iss(mSections[sectionIdx].subsections[idx].value);
        if (iss >> ret)
            return ret;

        return 0;
    }

    RoxMath::Vector4 RTextParser::getSubsectionValueVec4(int sectionIdx, int idx) const
    {
        if (idx < 0 || idx >= getSubsectionsCount(sectionIdx))
            return RoxMath::Vector4();

        return vec4FromString(mSections[sectionIdx].subsections[idx].value.c_str());
    }

    RoxMath::Vector4 RTextParser::getSubsectionValueVec4(int sectionIdx, const char* type) const
    {
        if (!type)
            return RoxMath::Vector4();

        for (int i = 0; i < getSubsectionsCount(sectionIdx); ++i)
        {
            if (mSections[sectionIdx].subsections[i].type == type)
                return vec4FromString(mSections[sectionIdx].subsections[i].value.c_str());
        }

        return RoxMath::Vector4();
    }

    RTextParser::Line RTextParser::Line::first(const char* text, std::size_t textSize)
    {
        Line l;
        l.text = text;
        l.textSize = textSize;
        l.offset = l.size = 0;
        l.global = l.empty = false;
        l.lineNumber = l.nextLineNumber = 1;
        return l;
    }

    // Line knows about quotes, '\n' characters inside quotes are not treated as new lines
    bool RTextParser::Line::next()
    {
        if (offset + size >= textSize)
            return false;

        offset += size;
        lineNumber = nextLineNumber;

        // Calculate line size, keep in mind multiline quoted tokens
        std::size_t charIdx = offset;
        bool inQuotes = false;
        while (charIdx != textSize)
        {
            char c = text[charIdx++];
            if (c == '\n')
            {
                ++nextLineNumber;
                if (!inQuotes)
                    break;
            }
            else if (c == '"')
                inQuotes = !inQuotes;
        }

        size = charIdx - offset;

        std::size_t firstNonWhitespaceIdx = skipWhitespaces(text, textSize, offset);
        global = (firstNonWhitespaceIdx < offset + size && text[firstNonWhitespaceIdx] == globalMarker);
        empty = (firstNonWhitespaceIdx >= offset + size);

        return true;
    }

    bool RTextParser::loadFromData(const char* text, std::size_t textSize)
    {
        if (!text)
            return false;

        textSize = getRealTextSize(text, textSize);
        if (!textSize)
            return true;

        // Removing comments
        std::string uncommentedText(text, textSize);
        while (uncommentedText.find("//") != std::string::npos)
        {
            std::size_t from = uncommentedText.find("//");
            std::size_t to = uncommentedText.find_first_of("\n\r", from + 1);
            if (to == std::string::npos)
                to = uncommentedText.size();
            uncommentedText.erase(from, to - from);
        }
        while (uncommentedText.find("/*") != std::string::npos)
        {
            std::size_t from = uncommentedText.find("/*");
            std::size_t to = uncommentedText.find("*/", from + 2);
            if (to == std::string::npos)
                to = uncommentedText.size();
            else
                to += 2;
            uncommentedText.erase(from, to - from);
        }
        text = uncommentedText.c_str();
        textSize = uncommentedText.size();

        std::size_t globalCount = 0;
        Line l = Line::first(text, textSize);
        while (l.next())
            if (l.global)
                ++globalCount;
        mSections.resize(globalCount);

        std::size_t subsectionStartIdx = 0, subsectionEndIdx = 0, sectionsCount = 0;
        bool subsectionEmpty = true;
        l = Line::first(text, textSize);

        while (l.next())
        {
            if (l.global)
            {
                if (subsectionEndIdx > subsectionStartIdx && !subsectionEmpty)
                {
                    mSections[sectionsCount - 1].value = std::string(text + subsectionStartIdx, subsectionEndIdx - subsectionStartIdx);
                    subsectionEmpty = true;
                }
                fillSection(mSections[sectionsCount], l);
                subsectionStartIdx = l.offset + l.size;
                ++sectionsCount;
            }
            else
            {
                if (sectionsCount > 0)
                {
                    subsectionEndIdx = l.offset + l.size;
                    if (!l.empty)
                        subsectionEmpty = false;
                }
                else if (!l.empty)
                    RoxLogger::log() << "RTextParser: subsection found before any section declaration at lines " << l.lineNumber << "-" << l.nextLineNumber << "\n";
            }
        }

        if (subsectionEndIdx > subsectionStartIdx && !subsectionEmpty)
            mSections[sectionsCount - 1].value = std::string(text + subsectionStartIdx, subsectionEndIdx - subsectionStartIdx);

        return true;
    }

    void RTextParser::fillSection(Section& s, const Line& l)
    {
        std::list<std::string> tokens = tokenizeLine(l);
        if (tokens.empty())
            return;

        auto iter = tokens.begin();
        s.type.swap(*iter++);
        bool needOption = false;
        bool needValue = false;
        bool needName = true;
        while (iter != tokens.end())
        {
            if (needOption)
            {
                s.option.swap(*iter);
                needOption = false;
            }
            else if (*iter == ":")
            {
                needOption = true;
                needName = false;
                needValue = false;
            }
            else if (*iter == "=")
            {
                needValue = true;
                needName = false;
            }
            else if (needValue)
            {
                s.value.append(*iter);
            }
            else if (needName)
            {
                if (s.names.empty() || !s.names.back().empty())
                    s.names.emplace_back();

                s.names.back().swap(*iter);
            }
            else
            {
                RoxLogger::log() << "RTextParser: unexpected token at lines " << l.lineNumber << "-" << l.nextLineNumber << "\n";
                break;
            }

            ++iter;
        }
    }

    std::list<std::string> RTextParser::tokenizeLine(const Line& l)
    {
        std::list<std::string> result;
        std::size_t lineEnd = l.offset + l.size;
        std::size_t charIdx = l.offset;

        while (true)
        {
            std::size_t tokenStartIdx, tokenSize;
            charIdx = getNextToken(l.text, lineEnd, charIdx, tokenStartIdx, tokenSize);
            if (tokenStartIdx < lineEnd)
                result.emplace_back(std::string(l.text + tokenStartIdx, tokenSize));
            else
                break;
        }

        return result;
    }

    std::size_t RTextParser::getRealTextSize(const char* text, std::size_t supposedSize)
    {
        const char* t = text;
        if (supposedSize != noSize)
            while (*t && t < text + supposedSize)
                ++t;
        else
            while (*t)
                ++t;

        return t - text;
    }

    std::size_t RTextParser::getNextToken(const char* text, std::size_t textSize, std::size_t pos, std::size_t& tokenStartIdxOut, std::size_t& tokenSizeOut)
    {
        std::size_t charIdx = pos;
        charIdx = skipWhitespaces(text, textSize, charIdx);
        if (charIdx < textSize && std::strchr(specialChars, text[charIdx]))
        {
            tokenStartIdxOut = charIdx;
            tokenSizeOut = 1;
            return charIdx + 1;
        }

        if (charIdx >= textSize)
        {
            tokenStartIdxOut = textSize;
            tokenSizeOut = 0;
            return textSize;
        }

        tokenStartIdxOut = charIdx;
        bool quotedToken = false;
        if (text[charIdx] == '"')
        {
            ++charIdx;
            tokenStartIdxOut = charIdx;
            quotedToken = true;
        }

        std::size_t tokenEndIdx = tokenStartIdxOut;
        bool endFound = false;
        while (charIdx < textSize && !endFound)
        {
            char c = text[charIdx];
            if (quotedToken)
            {
                if (c == '"')
                {
                    tokenEndIdx = charIdx;
                    endFound = true;
                }

                ++charIdx;
            }
            else
            {
                if (c <= ' ' || std::strchr(specialChars, c))
                {
                    tokenEndIdx = charIdx;
                    endFound = true;
                }
                else
                    ++charIdx;
            }
        }

        tokenSizeOut = (endFound ? tokenEndIdx : textSize) - tokenStartIdxOut;

        return charIdx;
    }

    std::size_t RTextParser::skipWhitespaces(const char* text, std::size_t textSize, std::size_t pos)
    {
        while (pos < textSize && text[pos] <= ' ')
            ++pos;

        return pos;
    }

    std::size_t RTextParser::skipWhitespacesBack(const char* text, std::size_t pos)
    {
        for (std::size_t i = 0; i <= pos; ++i)
        {
            if (text[pos - i] > ' ')
                return pos - i + 1;
        }

        return 0;
    }

} // namespace RoxFormats
