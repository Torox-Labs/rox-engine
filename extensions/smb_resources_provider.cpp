//nya-engine (C) nyan.developer@gmail.com released under the MIT license (see LICENSE)

//requires: libsmb https://videolabs.github.io/libdsm/ depends on libtasn1

#include "smb_resources_provider.h"

#include <arpa/inet.h>
extern "C" {
#include "bdsm.h"
}

namespace nya_resources
{

bool smb_resources_provider::open(const char *url,const char *user,const char *pwd)
{
    close();
    if(!url || !url[0])
        return false;

    std::string host(url),folder,path;
    const size_t slash=host.find('/');
    if(slash==std::string::npos)
    {
        log()<<"Invalid path: "<<host<<"\n";
        return false;
    }

    const size_t slash2=host.find('/',slash+2);
    if(slash2!=std::string::npos)
    {
        path=host.substr(slash2+1);
        folder=host.substr(slash+1,slash2-slash-1);
    }
    else
        folder=host.substr(slash+1);
    host.resize(slash);

    if(!path.empty() && path[path.length()-1]!='/')
        path.push_back('/');

    netbios_ns *ns=netbios_ns_new();
    if(!ns)
        return false;

    uint32_t addr;
    sockaddr_in sa;
    if(inet_pton(AF_INET,host.c_str(),&(sa.sin_addr))>0)
    {
        const char *chost=netbios_ns_inverse(ns,sa.sin_addr.s_addr);
        netbios_ns_destroy(ns);
        if(!chost)
        {
            log()<<"Unable to resolve smb: "<<host<<"\n";
            return false;
        }
        addr=sa.sin_addr.s_addr;
        host=chost;
    }
    else
    {
        size_t dot=host.rfind('.');
        if(dot!=std::string::npos)
            host.resize(dot);
        const int result=netbios_ns_resolve(ns,host.c_str(),NETBIOS_FILESERVER,&addr);
        netbios_ns_destroy(ns);
        if(result!=0)
        {
            log()<<"Unable to resolve smb: "<<host<<"\n";
            return false;
        }
    }

    smb_session *session=smb_session_new();
    if(!session)
        return false;

    if(smb_session_connect(session,host.c_str(),addr,SMB_TRANSPORT_TCP)!=0)
    {
        smb_session_destroy(session);
        log()<<"Unable to connect to host: "<<host<<"\n";
        return false;
    }

    smb_session_set_creds(session,host.c_str(),user,pwd);
    int result=smb_session_login(session);
    if(result!=0)
    {
        smb_session_destroy(session);
        log()<<"Auth failed ("<<result<<")\n";
        return false;
    }

    if(user && !(strcmp(user,"guest")==0))
    {
        int is_guest=smb_session_is_guest(session)==1;
        if(is_guest)
            log()<<"Warning: logged as guest\n";
    }

    smb_tid tid;
    result=smb_tree_connect(session,folder.c_str(),&tid);
    if(result!=0)
    {
        smb_session_destroy(session);
        log()<<"Unable to connect to share ("<<result<<")\n";
        return false;
    }

    nya_memory::lock_guard_write lock(m_mutex);

    m_session=session;
    m_tid=tid;
    m_path=path;
    return true;
}

void smb_resources_provider::close()
{
    nya_memory::lock_guard_write lock(m_mutex);

    if(m_session)
    {
        if(m_tid>0)
            smb_tree_disconnect(m_session,m_tid);
        smb_session_destroy(m_session);
    }
    m_tid=0;
    m_session=0;
    m_path.clear();
    m_resource_names.clear();
}

bool smb_resources_provider::list_dir(std::string subpath)
{
    if(subpath.empty())
        m_resource_names.clear();

    const smb_stat_list files=smb_find(m_session,m_tid,(m_path+subpath+"*").c_str());
    if(!files)
        return false;

    const int count=(int)smb_stat_list_count(files);
    for(int i=0;i<count;++i)
    {
        const smb_stat st=smb_stat_list_at(files, i);
        if(!st)
            continue;

        const char *name=smb_stat_name(st);
        if(name && name[0] && name[0]!='.')
        {
            bool is_dir=smb_stat_get(st,SMB_STAT_ISDIR)>0;
            if(is_dir)
                list_dir(subpath+name+"/");
            else
                m_resource_names.push_back(subpath+name);
        }
    }
    smb_stat_list_destroy(files);
    return true;
}

int smb_resources_provider::get_resources_count()
{
    m_mutex.lock_read();
    const int result=(int)m_resource_names.size();
    m_mutex.unlock_read();
    if(result>0)
        return result;

    nya_memory::lock_guard_write lock(m_mutex);
    if(!m_session)
        return 0;

    list_dir();
    return (int)m_resource_names.size();
}

const char *smb_resources_provider::get_resource_name(int idx)
{
    if(idx<0 || idx>=get_resources_count())
        return 0;

    nya_memory::lock_guard_read lock(m_mutex);
    return m_resource_names[idx].c_str();
}

bool smb_resources_provider::has(const char *resource_name)
{
    nya_memory::lock_guard_write lock(m_mutex);

    if(!m_session || !resource_name || !resource_name[0])
        return false;

    const smb_stat s=smb_fstat(m_session,m_tid,(m_path+resource_name).c_str());
    if(!s)
        return false;
    smb_stat_destroy(s);
    return true;
}

namespace
{

class smb_resource: public resource_data
{
public:
    smb_session *m_session;
    smb_fd m_fd;
    size_t m_size,m_off;

public:
    size_t get_size() { return m_size; }

    bool read_all(void*d) { return read_chunk(d,m_size,0); }

    bool read_chunk(void *d,size_t size,size_t offset)
    {
        if(!d || offset+size>m_size)
            return false;

        if(!size)
            return true;

        if(m_off!=offset)
            smb_fseek(m_session,m_fd,m_off,SMB_SEEK_SET);
        const size_t read=smb_fread(m_session,m_fd,d,size);
        m_off=offset+read;
        return read==size;
    }

    void release() { smb_fclose(m_session,m_fd); delete this; }

    smb_resource(smb_session *session,smb_fd fd): m_session(session),m_fd(fd),m_off(0)
    {
        const smb_stat s=smb_stat_fd(m_session,fd);
        m_size=s?smb_stat_get(s,SMB_STAT_SIZE):0;
    }
};

}

resource_data *smb_resources_provider::access(const char *resource_name)
{
    nya_memory::lock_guard_write lock(m_mutex);

    if(!m_session || !resource_name || !resource_name[0])
        return 0;

    smb_fd fd;
    if(smb_fopen(m_session,m_tid,(m_path+resource_name).c_str(),SMB_MOD_RO,&fd)!=0)
        return 0;

    return new smb_resource(m_session,fd);
}

}
