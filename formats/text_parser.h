//nya-engine (C) nyan.developer@gmail.com released under the MIT license (see LICENSE)

#pragma once

#include "math/vector.h"
#include <string>
#include <vector>
#include <list>

namespace nya_log { class ostream_base; }

namespace nya_formats
{

class text_parser
{
public:
    static const size_t no_size=(size_t)-1;
    bool load_from_data(const char *data,size_t text_size=no_size);

public:
    int get_sections_count() const { return (int)m_sections.size(); }
    const char *get_section_type(int idx) const;
    bool is_section_type(int idx,const char *type) const;
    int get_section_names_count(int idx) const;
    const char *get_section_name(int idx,int name_idx=0) const;
    const char *get_section_option(int idx) const;
    const char *get_section_value(int idx) const;
    nya_math::vec4 get_section_value_vec4(int idx) const;

public:
    int get_subsections_count(int section_idx) const;
    const char *get_subsection_type(int section_idx,int idx) const;
    const char *get_subsection_value(int section_idx,int idx) const;
    const char *get_subsection_value(int section_idx,const char *type) const;
    bool get_subsection_value_bool(int section_idx,int idx) const;
    int get_subsection_value_int(int section_idx,int idx) const;
    nya_math::vec4 get_subsection_value_vec4(int section_idx,int idx) const;
    nya_math::vec4 get_subsection_value_vec4(int section_idx,const char *type) const;

public:
    void debug_print(nya_log::ostream_base &os) const;

public:
    text_parser() {}
    text_parser(const char *data,size_t size=no_size) { load_from_data(data,size); }

private:
    void clear() { m_sections.clear(); }

    struct subsection
    {
        std::string type;
        std::string value;
    };

    struct section
    {
        std::string type;
        std::vector<std::string> names;
        std::string option;
        std::string value;
        // value -> subsections conversion is done on first subsection access for this section
        mutable bool subsection_parsed;
        mutable std::vector<subsection> subsections;

        section(): subsection_parsed(false) { names.resize(1); }
    };

    std::vector<section> m_sections;

private:
    struct line
    {
        const char *text;
        size_t text_size;
        size_t offset;
        size_t size;
        bool global;
        bool empty;
        size_t line_number;
        size_t next_line_number;

        static line first(const char *text,size_t text_size);
        bool next();
    };

    static size_t get_real_text_size(const char *text,size_t supposed_size);
    static std::list<std::string> tokenize_line(const line &l);
    static void fill_section(section &s, const line &l);
    // As text is NOT null-terminated but size-constrained string we should provide following output parameters:
    // 1) start index and size of found token.
    // 2) idx of last symbol processed during this token processing, which can be used for the following text processing (this number is not necessary equals token_start_idx + token_size due to quotes magic).
    // token_start_idx==text_size serves as 'no token found' mark, token_size==0 indicates zero-sized token (consider @param = "").
    static size_t get_next_token(const char *text,size_t text_size,size_t pos,size_t &token_start_idx_out,size_t &token_size_out);
    static size_t skip_whitespaces(const char *text,size_t text_size,size_t pos);
    static size_t skip_whitespaces_back(const char *text,size_t pos);
};

}
