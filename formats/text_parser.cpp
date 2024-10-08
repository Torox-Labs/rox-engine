//nya-engine (C) nyan.developer@gmail.com released under the MIT license (see LICENSE)

#include "text_parser.h"
#include "log/log.h"
#include "memory/invalid_object.h"
#include "string_convert.h"

#include <iostream>
#include <sstream>
#include <algorithm>
#include <cstring>

#ifdef _MSC_VER
    #define strcasecmp _stricmp
#endif

namespace
{
    const char global_marker='@';
    const char *special_chars="=:";
}

namespace nya_formats
{

const char *text_parser::get_section_type(int idx) const
{
    if(idx<0 || idx>=(int)m_sections.size())
        return 0;

    return m_sections[idx].type.c_str();
}

bool text_parser::is_section_type(int idx,const char *type) const
{
    if(!type || idx<0 || idx>=(int)m_sections.size())
        return false;

    if(type[0]=='@')
        return m_sections[idx].type==type;

    return m_sections[idx].type.compare(1,m_sections[idx].type.size()-1,type)==0;
}

int text_parser::get_section_names_count(int idx) const
{
    if(idx < 0 || idx >= (int)m_sections.size())
        return 0;

    return (int)m_sections[idx].names.size();
}

const char *text_parser::get_section_name(int idx,int name_idx) const
{
    if(idx<0 || idx>=(int)m_sections.size())
        return 0;

    if(name_idx<0 || name_idx>=(int)m_sections[idx].names.size())
        return 0;

    return m_sections[idx].names[name_idx].c_str();
}

const char *text_parser::get_section_option(int idx) const
{
    if(idx<0 || idx>=(int)m_sections.size())
        return 0;

    return m_sections[idx].option.c_str();
}

const char *text_parser::get_section_value(int idx) const
{
    if(idx<0 || idx>=(int)m_sections.size())
        return 0;

    return m_sections[idx].value.c_str();
}

nya_math::vec4 text_parser::get_section_value_vec4(int idx) const
{
    if(idx<0 || idx>=(int)m_sections.size())
        return nya_memory::invalid_object<nya_math::vec4>();

    return vec4_from_string(m_sections[idx].value.c_str());
}

inline void remove_quotes(std::string &s)
{
    if(s.empty())
        return;

    if(s[0]=='"' && s[s.size()-1]==s[0])
        s=s.substr(1,s.size()-2);
}

int text_parser::get_subsections_count(int section_idx) const
{
    if(section_idx<0 || section_idx>=(int)m_sections.size())
        return -1;

    const section &s=m_sections[section_idx];
    if(!s.subsection_parsed)
    {
        // parse subsection
        line l=line::first(s.value.c_str(),s.value.size());
        while(l.next())
        {
            const char *text=l.text+l.offset;
            const size_t off=skip_whitespaces(text,l.size,0);
            if(off==l.size)
                continue;

            text+=off;
            const size_t roff=skip_whitespaces_back(text,l.size-off-1);

            size_t eq=std::string::npos;
            for(size_t i=0;i<roff;++i)
            {
                if(text[i]=='=')
                {
                    eq=i;
                    break;
                }
            }

            if(!eq)
                continue;

            s.subsections.push_back(subsection());
            subsection &ss=s.subsections.back();

            if(eq==std::string::npos)
            {
                ss.type=std::string(text,roff);
                remove_quotes(ss.type);
                continue;
            }

            ss.type=std::string(text,skip_whitespaces_back(text,eq-1));
            remove_quotes(ss.type);
            eq=skip_whitespaces(text,roff,eq+1);
            ss.value=std::string(text+eq,roff-eq);
            remove_quotes(ss.value);
        }

        s.subsection_parsed=true;
    }

    return (int)s.subsections.size();
}

const char *text_parser::get_subsection_type(int section_idx,int idx) const
{
    if(idx<0 || idx>=get_subsections_count(section_idx))
        return 0;

    return m_sections[section_idx].subsections[idx].type.c_str();
}

const char *text_parser::get_subsection_value(int section_idx,int idx) const
{
    if(idx<0 || idx>=get_subsections_count(section_idx))
        return 0;

    return m_sections[section_idx].subsections[idx].value.c_str();
}

const char *text_parser::get_subsection_value(int section_idx,const char *type) const
{
    if(!type)
        return 0;

    for(int i=0;i<get_subsections_count(section_idx);++i)
    {
        if(m_sections[section_idx].subsections[i].type==type)
            return m_sections[section_idx].subsections[i].value.c_str();
    }

    return 0;
}

bool text_parser::get_subsection_value_bool(int section_idx,int idx) const
{
    if(idx<0 || idx>=get_subsections_count(section_idx))
        return 0;

    return bool_from_string(m_sections[section_idx].subsections[idx].value.c_str());
}

int text_parser::get_subsection_value_int(int section_idx,int idx) const
{
    if(idx<0 || idx>=get_subsections_count(section_idx))
        return 0;

    int ret=0;
    std::istringstream iss(m_sections[section_idx].subsections[idx].value);
    if(iss>>ret)
        return ret;

    return 0;
}

nya_math::vec4 text_parser::get_subsection_value_vec4(int section_idx,int idx) const
{
    if(idx<0 || idx>=get_subsections_count(section_idx))
        return nya_math::vec4();

    return vec4_from_string(m_sections[section_idx].subsections[idx].value.c_str());
}

nya_math::vec4 text_parser::get_subsection_value_vec4(int section_idx,const char *type) const
{
    if(!type)
        return nya_math::vec4();

    for(int i=0;i<get_subsections_count(section_idx);++i)
    {
        if(m_sections[section_idx].subsections[i].type==type)
            return vec4_from_string(m_sections[section_idx].subsections[i].value.c_str());
    }

    return nya_math::vec4();
}

text_parser::line text_parser::line::first(const char *text,size_t text_size)
{
    line l;
    l.text=text;
    l.text_size=text_size;
    l.offset=l.size=0;
    l.global=l.empty=false;
    l.line_number=l.next_line_number=1;
    return l;
}

// line knows about quotes, '\n' characters inside quotes are not treated as new line
bool text_parser::line::next()
{
    if(offset+size>=text_size)
        return false;

    offset+=size;
    line_number=next_line_number;

    // calculate line size, keep in mind multiline quoted tokens
    size_t char_idx=offset;
    bool in_quotes=false;
    while(char_idx!=text_size)
    {
        char c=text[char_idx++];
        if(c=='\n')
        {
            ++next_line_number;
            if(!in_quotes)
                break;
        }
        else if(c=='"')
            in_quotes=!in_quotes;
    }

    size=char_idx-offset;

    const size_t first_non_whitespace_idx=skip_whitespaces(text,text_size,offset);
    global=(first_non_whitespace_idx<offset+size && text[first_non_whitespace_idx]==global_marker);
    empty=(first_non_whitespace_idx>=offset+size);

    return true;
}

bool text_parser::load_from_data(const char *text,size_t text_size)
{
    if(!text)
        return false;

    text_size=get_real_text_size(text,text_size);
    if(!text_size)
        return true;

    //removing comments
    std::string uncommented_text(text,text_size);
    while(uncommented_text.find("//")!=std::string::npos)
    {
        const size_t from=uncommented_text.find("//");
        uncommented_text.erase(from,uncommented_text.find_first_of("\n\r",from+1)-from);
    }
    while(uncommented_text.find("/*")!=std::string::npos)
    {
        const size_t from=uncommented_text.find("/*");
        uncommented_text.erase(from,(uncommented_text.find("*/",from+2)-from)+2);
    }
    text=uncommented_text.c_str();
    text_size=uncommented_text.size();

    size_t global_count=0;
    line l=line::first(text,text_size);
    while(l.next()) if(l.global) ++global_count;
    m_sections.resize(global_count);

    size_t subsection_start_idx=0,subsection_end_idx=0,sections_count=0;
    bool subsection_empty=true;
    l=line::first(text,text_size);

    while(l.next())
    {
        if(l.global)
        {
            if(subsection_end_idx>subsection_start_idx && !subsection_empty)
            {
                m_sections[sections_count-1].value=std::string(text+subsection_start_idx,subsection_end_idx-subsection_start_idx);
                subsection_empty=true;
            }
            fill_section(m_sections[sections_count],l);
            subsection_start_idx=l.offset+l.size;
            ++sections_count;
        }
        else
        {
            if(sections_count>0)
            {
                subsection_end_idx=l.offset+l.size;
                if(!l.empty)
                    subsection_empty=false;
            }
            else if(!l.empty)
                nya_log::log()<<"Text parser: subsection found before any section declaration at lines "<< l.line_number<<"-"<<l.next_line_number<<"\n";
        }
    }

    if(subsection_end_idx>subsection_start_idx && !subsection_empty)
        m_sections[sections_count-1].value=std::string(text+subsection_start_idx,subsection_end_idx-subsection_start_idx);

    return true;
}

void text_parser::fill_section(section &s,const line &l)
{
   std::list<std::string> tokens=tokenize_line(l);
    // assert(tokens.size() > 0);
    // assert(tokens.front().size > 0);
    // assert(tokens.front().at(0) == global_marker);
    std::list<std::string>::iterator iter=tokens.begin();
    s.type.swap(*(iter++));
    bool need_option=false;
    bool need_value=false;
    bool need_name=true;
    while(iter!=tokens.end())
    {
        if(need_option)
        {
            s.option.swap(*iter);
            need_option=false;
        }
        else if(*iter==":")
        {
            need_option=true;
            need_name=false;
            need_value=false;
        }
        else if(*iter=="=")
        {
            need_value=true;
            need_name=false;
        }
        else if(need_value)
        {
            s.value.append(*iter);
        }
        else if(need_name)
        {
            if(s.names.empty() || !s.names.back().empty())
                s.names.push_back(std::string());

            s.names.back().swap(*iter);
        }
        else
        {
            nya_log::log()<<"Text parser: unexpected token at lines "<<l.line_number<<"-"<<l.next_line_number<<"\n";
            break;
        }

        ++iter;
    }
}

std::list<std::string> text_parser::tokenize_line(const line &l)
{
    std::list<std::string> result;
    const size_t line_end=l.offset+l.size;
    size_t char_idx=l.offset;

    while(true)
    {
        size_t token_start_idx, token_size;
        char_idx=get_next_token(l.text,line_end,char_idx,token_start_idx,token_size);
        if(token_start_idx<line_end)
            result.push_back(std::string(l.text+token_start_idx,token_size));
        else
            break;
    }

    return result;
}

size_t text_parser::get_real_text_size(const char *text,size_t supposed_size)
{
    const char *t=text;
    if(supposed_size!=no_size)
        while(*t && t<text+supposed_size) ++t;
    else
        while(*t) ++t;

    return t-text;
}

size_t text_parser::get_next_token(const char *text,size_t text_size,size_t pos,size_t &token_start_idx_out,size_t &token_size_out)
{
    size_t char_idx=pos;
    char_idx=skip_whitespaces(text,text_size,char_idx);
    if(char_idx<text_size && strchr(special_chars,text[char_idx]))
    {
        token_start_idx_out=char_idx;
        token_size_out=1;
        return char_idx+1;
    }

    if(char_idx>=text_size)
    {
        token_start_idx_out=text_size;
        token_size_out=0;
        return text_size;
    }

    token_start_idx_out=char_idx;
    bool quoted_token=false;
    if(text[char_idx]=='"')
    {
        ++char_idx;
        token_start_idx_out=char_idx;
        quoted_token=true;
    }

    size_t token_end_idx=token_start_idx_out;
    bool end_found=false;
    while(char_idx<text_size && !end_found)
    {
        char c=text[char_idx];
        if(quoted_token)
        {
            if(c=='"')
            {
                token_end_idx=char_idx;
                end_found=true;
            }

            ++char_idx;
        }
        else
        {
            if(c<=' ' || strchr(special_chars, c))
            {
                token_end_idx=char_idx;
                end_found=true;
            }
            else
                ++char_idx;
        }
    }

    token_size_out=(end_found?token_end_idx:text_size)-token_start_idx_out;

    return char_idx;
}

size_t text_parser::skip_whitespaces(const char *text,size_t text_size,size_t pos)
{
    while(pos<text_size && text[pos]<=' ')
        ++pos;

    return pos;
}

size_t text_parser::skip_whitespaces_back(const char *text,size_t pos)
{
    for(size_t i=0;i<=pos;++i)
    {
        if(text[pos-i]>' ')
            return pos-i+1;
    }

    return 0;
}

void text_parser::debug_print(nya_log::ostream_base &os) const
{
    for(size_t i=0;i<m_sections.size();++i)
    {
        const section &s=m_sections[i];
        os<<"section "<<i<<" '"<<s.type<<"':\n";
        for(size_t j=0;j<s.names.size();++j)
            os<<"  name "<<j<<" '"<<s.names[j]<<"'\n";

        if(!s.option.empty())
            os<<"  option '"<<s.option<<"'\n";
        os<<"  value '"<<s.value<<"'\n\n\n";
    }
}

}
