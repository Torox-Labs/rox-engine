// Copyright © 2024 Torox Project
// Portions Copyright © 2013 nyan.developer@gmail.com (nya-engine)
//
// This file was modified by the Torox Project. The namespace has been renamed from nya_log to rox_log.
// 
// This file incorporates code from the nya-engine project, which is licensed under the MIT License.
// See the LICENSE-MIT file in the root directory for more information.
//
// This file is also part of the Rox-engine, which is licensed under a dual-license system:
// 1. Free Use License (for non-commercial and commercial use under specific conditions)
// 2. Commercial License (for use on proprietary platforms)
// See the LICENSE file in the root directory for the full Rox-engine license terms.

#pragma once

#include "logger.h"
#include <vector>

namespace rox_log
{

class warnings_counter
{
public:
    int add_warning(const char *msg);
    void clear() { m_warnings.clear(); }
    void ignore_warnings(bool ignore) { m_ignore_warnings=ignore; }

public:
    int get_unique_warnings_count() { return (int)m_warnings.size(); }
    const char *get_warning_message(int idx);
    unsigned int get_warnings_count(int idx);
    unsigned int get_total_warnings_count();

public:
    warnings_counter(): m_ignore_warnings(false) {}

private:
    typedef std::vector<std::pair<std::string,unsigned int> > warnings_counts_map;
    warnings_counts_map m_warnings;
    bool m_ignore_warnings;
};

class warning_ostream: public ostream_base
{
public:
    warning_ostream(warnings_counter &counter): m_counter(counter) {}
    virtual ~warning_ostream() { flush(); }

    void flush();

private:
    virtual void output(const char *str);

private:
    std::string m_buf;
    warnings_counter &m_counter;
};

warnings_counter &get_warnings_counter();
warning_ostream warning();

}
