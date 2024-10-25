//nya-engine (C) nyan.developer@gmail.com released under the MIT license (see LICENSE)

//requires: curl https://curl.haxx.se/

#include "ftp_resources_provider.h"
#include "curl/curl.h"

//ToDo: log

namespace nya_resources
{

static size_t write_response(void *ptr,size_t size,size_t count,void *data)
{
    std::vector<char> &d=*static_cast<std::vector<char>*>(data);

    const size_t part_size=size*count;
    const size_t off=d.size();

    d.resize(off+part_size,0);
    memcpy(&d[off],ptr,part_size);

    return part_size;
}

static bool get_data(CURL *c,const std::string &url,std::vector<char> &data)
{
    if(!c)
        return false;

    curl_easy_setopt(c,CURLOPT_URL,url.c_str());
    curl_easy_setopt(c,CURLOPT_WRITEDATA,&data);
    curl_easy_setopt(c,CURLOPT_WRITEFUNCTION,write_response);
    return curl_easy_perform(c)==CURLE_OK;
}

bool ftp_resources_provider::open(const char *url,const char *userpwd)
{
    if(!url || !url[0])
        return false;

    m_base_url.assign(url);
    if(m_base_url.back()!='/')
        m_base_url.push_back('/');

    if(!m_curl)
        m_curl=curl_easy_init();

    if(!m_curl)
        return false;

    if(userpwd)
        curl_easy_setopt(m_curl,CURLOPT_USERPWD,userpwd);

    if(!get_entries(""))
    {
        close();
        return false;
    }

    return true;
}

bool ftp_resources_provider::get_entries(const char *folder)
{
    curl_easy_setopt(m_curl,CURLOPT_DIRLISTONLY,1);

    std::vector<char> data;
    if(!get_data(m_curl,m_base_url+folder,data))
        return false;

    for(size_t i=0,prev=0;i<data.size();++i)
    {
        if(data[i]=='\n')
        {
            const std::string name=folder+std::string(&data[prev],i-prev);
            if(!get_entries((name+'/').c_str()))
                m_entries.push_back(name);
            prev=i+1;
        }
    }

    return true;
}

void ftp_resources_provider::close()
{
    m_entries.clear();
    if(!m_curl)
        return;

    curl_easy_cleanup(m_curl);
    m_curl=0;
    m_base_url.clear();
}

int ftp_resources_provider::get_resources_count() { return (int)m_entries.size(); }
const char *ftp_resources_provider::get_resource_name(int idx)
{
    if(idx<0 || idx>=get_resources_count())
        return 0;

    return m_entries[idx].c_str();
}

bool ftp_resources_provider::has(const char *resource_name)
{
    if(!resource_name || !resource_name[0])
        return false;

    for(int i=0;i<(int)m_entries.size();++i)
    {
        if(m_entries[i]==resource_name)
            return true;
    }

    return false;
}

namespace
{

class ftp_resource: public resource_data
{
public:
    std::vector<char> data;

public:
    size_t get_size() { return data.size(); }

    bool read_all(void*d)
    {
        if(!d)
            return false;

        if(data.empty())
            return true;

        return memcpy(d,&data[0],data.size());
    }

    bool read_chunk(void *d,size_t size,size_t offset)
    {
        if(!d || offset+size>data.size())
            return false;

        if(data.empty() && offset==0)
            return true;

        if(offset>=data.size())
            return false;

        return memcpy(d,&data[offset],size);
    }

    void release() { delete this; }
};

}

resource_data *ftp_resources_provider::access(const char *resource_name)
{
    if(!resource_name || !resource_name[0])
        return 0;

    for(int i=0;i<get_resources_count();++i)
    {
        if(m_entries[i]==resource_name)
        {
            curl_easy_setopt(m_curl,CURLOPT_DIRLISTONLY,0);

            ftp_resource *f=new ftp_resource();
            if(!get_data(m_curl,m_base_url+resource_name,f->data))
            {
                delete f;
                return 0;
            }
            return f;
        }
    }

    return 0;
}

}
