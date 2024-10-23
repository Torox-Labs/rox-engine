// nya-engine (C) nyan.developer@gmail.com released under the MIT license (see LICENSE)

#pragma once

#include "RoxMath/RoxVector.h"
#include "RoxRender/RoxRender.h"
#include <string>
#include <vector>
#include <list>

namespace RoxLogger { class RoxOstreamBase; }

namespace RoxFormats
{

    class RTextParser
    {
    public:
        static const std::size_t noSize = static_cast<std::size_t>(-1);
        bool loadFromData(const char* data, std::size_t textSize = noSize);

    public:
        int getSectionsCount() const { return static_cast<int>(mSections.size()); }
        const char* getSectionType(int idx) const;
        bool isSectionType(int idx, const char* type) const;
        int getSectionNamesCount(int idx) const;
        const char* getSectionName(int idx, int nameIdx = 0) const;
        const char* getSectionOption(int idx) const;
        const char* getSectionValue(int idx) const;
        RoxMath::Vector4 getSectionValueVec4(int idx) const;

    public:
        int getSubsectionsCount(int sectionIdx) const;
        const char* getSubsectionType(int sectionIdx, int idx) const;
        const char* getSubsectionValue(int sectionIdx, int idx) const;
        const char* getSubsectionValue(int sectionIdx, const char* type) const;
        bool getSubsectionValueBool(int sectionIdx, int idx) const;
        int getSubsectionValueInt(int sectionIdx, int idx) const;
        RoxMath::Vector4 getSubsectionValueVec4(int sectionIdx, int idx) const;
        RoxMath::Vector4 getSubsectionValueVec4(int sectionIdx, const char* type) const;

    public:
        void debugPrint(RoxLogger::RoxOstreamBase& os) const;

    public:
        RTextParser() {}
        RTextParser(const char* data, std::size_t size = noSize) { loadFromData(data, size); }

    private:
        void clear() { mSections.clear(); }

        struct Subsection
        {
            std::string type;
            std::string value;
        };

        struct Section
        {
            std::string type;
            std::vector<std::string> names;
            std::string option;
            std::string value;
            // value -> subsections conversion is done on first subsection access for this section
            mutable bool subsectionParsed;
            mutable std::vector<Subsection> subsections;

            Section() : subsectionParsed(false) { names.resize(1); }
        };

        std::vector<Section> mSections;

    private:
        struct Line
        {
            const char* text;
            std::size_t textSize;
            std::size_t offset;
            std::size_t size;
            bool global;
            bool empty;
            std::size_t lineNumber;
            std::size_t nextLineNumber;

            static Line first(const char* text, std::size_t textSize);
            bool next();
        };

        static std::size_t getRealTextSize(const char* text, std::size_t supposedSize);
        static std::list<std::string> tokenizeLine(const Line& l);
        static void fillSection(Section& s, const Line& l);
        // As text is NOT null-terminated but size-constrained string we should provide following output parameters:
        // 1) start index and size of found token.
        // 2) idx of last symbol processed during this token processing, which can be used for the following text processing (this number is not necessary equals token_start_idx + token_size due to quotes magic).
        // token_start_idx==text_size serves as 'no token found' mark, token_size==0 indicates zero-sized token (consider @param = "").
        static std::size_t getNextToken(const char* text, std::size_t textSize, std::size_t pos, std::size_t& tokenStartIdxOut, std::size_t& tokenSizeOut);
        static std::size_t skipWhitespaces(const char* text, std::size_t textSize, std::size_t pos);
        static std::size_t skipWhitespacesBack(const char* text, std::size_t pos);
    };

} // namespace RoxFormats
