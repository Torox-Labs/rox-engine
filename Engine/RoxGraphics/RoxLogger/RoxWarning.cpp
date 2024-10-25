// Copyright (C) 2024 Torox Project
// Portions Copyright (C) 2013 nyan.developer@gmail.com (nya-engine)
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

#include "RoxWarning.h"

namespace RoxLogger
{

int RoxWarningsCounter::addWarning(const char *msg)
{
    if(!msg || m_ignore_warnings)
        return 0;

    warnings_counts_map::iterator iter=m_warnings.begin();
    while(iter!=m_warnings.end() && iter->first!=msg)
        ++iter;

    if(iter==m_warnings.end())
        iter=m_warnings.insert(iter,std::make_pair(std::string(msg),0));

    ++(iter->second);
    return (int)(iter-m_warnings.begin());
}

unsigned int RoxWarningsCounter::getWarningsCount(int idx)
{
    if(idx<0 || idx>=(int)m_warnings.size())
        return 0;

    return m_warnings[idx].second;
}

const char *RoxWarningsCounter::getWarningMessage(int idx)
{
    if(idx<0 || idx>=(int)m_warnings.size())
        return 0;

    return m_warnings[idx].first.c_str();
}

unsigned int RoxWarningsCounter::getTotalWarningsCount()
{
    unsigned int count=0;
    for(size_t i=0;i<m_warnings.size();++i)
        count+=m_warnings[i].second;
    return count;
}

void RoxWarningOstream::flush()
{
    if(!m_buf.empty())
        m_counter.addWarning(m_buf.c_str());
}

void RoxWarningOstream::output(const char *str)
{
    m_buf.append(str?str:"NULL");
}

RoxWarningsCounter &getWarningsCounter() { static RoxWarningsCounter *wc=new RoxWarningsCounter(); return *wc; }
RoxWarningOstream warning() { return RoxWarningOstream( getWarningsCounter() ); }

}
