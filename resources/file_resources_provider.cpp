//nya-engine (C) nyan.developer@gmail.com released under the MIT license (see LICENSE)

#include "file_resources_provider.h"
#include "memory/pool.h"
#include "memory/lru.h"

#include <stdio.h>
#include <string.h>

#ifdef _WIN32
    #include <Windows.h>
	#include <io.h>
#else
	#include <dirent.h>
#endif

#include <sys/stat.h>

#ifndef S_ISDIR
	#define	S_ISDIR(m)	(((m) & S_IFMT) == S_IFDIR)
#endif

namespace nya_resources
{

class file_ref
{
public:
    void init(const char *name) { m_name.assign(name?name:""); }

    FILE *access() { nya_memory::lock_guard lock(m_mutex); return get_lru().access(m_name.c_str()); }

    void free() { nya_memory::lock_guard lock(m_mutex); get_lru().free(m_name.c_str()); }

    class lru: public nya_memory::lru<FILE *,64>
    {
        bool on_access(const char *name,FILE *& f) 
		{ 
		#ifdef _WIN32
			if(!name)
				return false;
			
			const int len=MultiByteToWideChar(CP_UTF8,0,name,-1,0,0);
			if(!len)
				return false;

			WCHAR *wname = new WCHAR[len];
			MultiByteToWideChar(CP_UTF8,0,name,-1,wname,len);
			f=_wfopen(wname, L"rb");
			delete wname;
			return f!=0;
		#else
			return name?(f=fopen(name,"rb"))!=0:false;
		#endif
		}
        bool on_free(const char *name,FILE *& f) { return fclose(f)==0; }
    };

    static lru &get_lru()
    {
        static lru *cache=new lru();
        return *cache;
    }

private:
    std::string m_name;
    nya_memory::mutex m_mutex;
};

class file_resource: public resource_data
{
public:
    size_t get_size() { return m_size; }

    bool read_all(void*data);
    bool read_chunk(void *data,size_t size,size_t offset);

public:
    bool open(const char*filename);
    void release();

    file_resource(): m_size(0) {}
    //~file_resource() { release(); }

private:
    file_ref m_file;
    size_t m_size;
};

}

namespace nya_resources
{

resource_data *file_resources_provider::access(const char *resource_name)
{
    if(!resource_name)
    {
        log()<<"unable to access file: invalid name\n";
        return 0;
    }

    file_resource *file = new file_resource;

    nya_memory::lock_guard_read lock(m_mutex);

    std::string file_name=m_path+resource_name;
    for(size_t i=m_path.size();i<file_name.size();++i)
    {
        if(file_name[i]=='\\')
            file_name[i]='/';
    }

    if(!file->open(file_name.c_str()))
    {
        log()<<"unable to access file: "<<file_name.c_str()+m_path.size()
                        <<" at path "<<m_path.c_str()<<"\n";
        file->release();
        return 0;
    }

    return file;
}

bool file_resources_provider::has(const char *name)
{
    if(!name)
        return false;

    nya_memory::lock_guard_read lock(m_mutex);

    std::string file_name=m_path+name;
    for(size_t i=m_path.size();i<file_name.size();++i)
    {
        if(file_name[i]=='\\')
            file_name[i]='/';
    }



#ifdef _WIN32
    const int len=MultiByteToWideChar(CP_UTF8,0,file_name.c_str(),-1,0,0);
    if(!len)
        return false;

    WCHAR *wname = new WCHAR[len];
    MultiByteToWideChar(CP_UTF8,0,file_name.c_str(),-1,wname,len);
    struct _stat sb;
    bool result=_wstat(wname,&sb)==0;
    delete wname;
    return result;
#else
    struct stat sb;
    return stat(file_name.c_str(),&sb)==0;
#endif
}

bool file_resources_provider::set_folder(const char*name,bool recursive,bool ignore_nonexistent)
{
    nya_memory::lock_guard_write lock(m_mutex);

    m_recursive=recursive;
    m_update_names=true;

    if(!name)
    {
        m_path.erase();
        return false;
    }

    m_path.assign(name);
    for(size_t i=0;i<m_path.size();++i)
    {
        if(m_path[i]=='\\')
            m_path[i]='/';
    }

    if(m_path.length()>2 && m_path[0]=='.' &&  m_path[1]=='/')
        m_path=m_path.substr(2);

    while(m_path.length()>1 && m_path[m_path.length()-1]=='/')
        m_path.resize(m_path.length()-1);

    if(m_path.empty())
        return true;

    if(!ignore_nonexistent)
    {
        struct stat sb;
        if(stat(m_path.c_str(),&sb)== -1)
        {
            log()<<"warning: unable to stat at path "<<name<<", probably does not exist\n";
            m_path.push_back('/');
            return false;
        }
        else if(!S_ISDIR(sb.st_mode))
        {
            log()<<"warning: specified path is not a directory "<<name<<"\n";
            m_path.push_back('/');
            return false;
        }
    }

    m_path.push_back('/');
    return true;
}

void file_resources_provider::enumerate_folder(const char*folder_name)
{
    if(!folder_name)
        return;

    std::string folder_name_str(folder_name);

    while(!folder_name_str.empty() && folder_name_str[folder_name_str.length()-1]=='/')
        folder_name_str.resize(folder_name_str.length()-1);

    std::string first_dir=(m_path+folder_name_str);
    if(first_dir.empty())
        return;

    while(!first_dir.empty() && first_dir[first_dir.length()-1]=='/')
        first_dir.resize(first_dir.length()-1);

#ifdef _WIN32
	_finddata_t data;
	intptr_t hdl=_findfirst((first_dir + "/*").c_str(),&data);
    if(!hdl)
#else
    DIR *dirp=opendir(first_dir.c_str());
    if(!dirp)
#endif
    {
        nya_log::log()<<"unable to enumerate folder "<<(m_path+folder_name_str).c_str()<<"\n";
        return;
    }

#ifdef _WIN32
	for(intptr_t it=hdl;it>=0;it=_findnext(hdl,&data))
	{
		const char *name=data.name;
#else
    while(dirent *dp=readdir(dirp))
    {
		const char *name=dp->d_name;
#endif
        if(strcmp(name,".")==0 || strcmp(name,"..")==0)
            continue;

        std::string entry_name;
        if(!folder_name_str.empty() && folder_name_str != ".")
            entry_name=folder_name_str+'/';
        entry_name.append(name);

#ifdef _WIN32
        const std::string full_path_str=m_path+entry_name;
        struct stat stat_buf;
        if(stat(full_path_str.c_str(),&stat_buf)<0)
        {
            nya_log::log()<<"unable to read "<<full_path_str.c_str()<<"\n";
            continue;
        }

        if((stat_buf.st_mode&S_IFDIR)==S_IFDIR && m_recursive)
#else
        if(dp->d_type==DT_DIR && m_recursive)
#endif
        {
            enumerate_folder(entry_name.c_str());
            continue;
        }

        m_resource_names.push_back(entry_name);
    }
#ifdef _WIN32
    _findclose(hdl);
#else
    closedir(dirp);
#endif
}

void file_resources_provider::update_names()
{
    m_resource_names.clear();
    enumerate_folder(m_path.empty()?".":"");
    m_update_names=false;
}

int file_resources_provider::get_resources_count()
{
    if(m_update_names)
        update_names();

    return (int)m_resource_names.size();
}

const char *file_resources_provider::get_resource_name(int idx)
{
    if(idx<0 || idx>=get_resources_count())
        return 0;

    return m_resource_names[idx].c_str();
}

void file_resources_provider::lock()
{
    resources_provider::lock();

    if(m_update_names)
    {
        resources_provider::unlock();
        m_mutex.lock_write();
        if(m_update_names)
            update_names();
        m_mutex.unlock_write();
        lock();
    }
}

bool file_resource::read_all(void*data)
{
    if(!data)
    {
        if(m_size>0)
            log()<<"unable to read file data: invalid data pointer\n";
        return false;
    }

    FILE *file=m_file.access();
    if(!file)
    {
        log()<<"unable to read file data: no such file\n";
        return false;
    }

    if(fseek(file,0,SEEK_SET)!=0)
    {
        log()<<"unable to read file data: seek_set failed\n";
        return false;
    }

    if(fread(data,1,m_size,file)!=m_size)
    {
        log()<<"unable to read file data: unexpected size of readen data\n";
        return false;
    }

    return true;
}

bool file_resource::read_chunk(void *data,size_t size,size_t offset)
{
    if(!data)
    {
        if(size>0)
            log()<<"unable to read file data chunk: invalid data pointer\n";
        return false;
    }

    FILE *file=m_file.access();
    if(!file)
    {
        log()<<"unable to read file data: no such file\n";
        return false;
    }

    if(offset+size>m_size||!size)
    {
        log()<<"unable to read file data chunk: invalid size\n";
        return false;
    }

    if(fseek(file,(long)offset,SEEK_SET)!=0)
    {
        log()<<"unable to read file data chunk: seek_set failed\n";
        return false;
    }

    if(fread(data,1,size,file)!=size)
    {
        log()<<"unable to read file data chunk: unexpected size of readen data\n";
        return false;
    }

    return true;
}

bool file_resource::open(const char*filename)
{
    m_file.free();

    m_size=0;

    if(!filename)
        return false;

    m_file.init(filename);
    FILE *file=m_file.access();
    if(!file)
        return false;

    if(fseek(file,0,SEEK_END)!=0)
        return false;

    m_size=ftell(file);

    return true;
}

void file_resource::release()
{
    m_file.free();
    delete this;
}

}
