//nya-engine (C) nyan.developer@gmail.com released under the MIT license (see LICENSE)

#pragma once

#include "resources.h"
#include "memory/pool.h"
#include <map>
#include <string>
#include <algorithm>

namespace nya_resources
{

template<typename t_res,int block_count> class shared_resources: public nya_memory::non_copyable
{
private:
    virtual bool fill_resource(const char *name,t_res &res) { return false; }
    virtual bool release_resource(t_res &res) { return false; }

private:
    class shared_resources_creator
    {
        template<typename,int> friend class shared_resources;
        struct res_holder;

        class shared_resource_ref
        {
            template<typename,int> friend class shared_resources;

        public:
            bool is_valid() const { return m_res!=0; }
            const t_res *const_get() const { return m_res; }
            const t_res *operator -> () const { return m_res; };

            const char *get_name() const
            {
                if(!m_creator)
                    return 0;

                return m_creator->get_res_name(*this);
            }

            int get_ref_count() const
            {
                if(!m_creator)
                    return 0;

                return m_creator->res_get_ref_count(*this);
            }

            void free()
            {
                if(m_creator)
                    m_creator->free(*this);

                m_res=0;
                m_res_holder=0;
                m_creator=0;
            }

        public:
            shared_resource_ref(): m_res(0), m_res_holder(0), m_creator(0) {}

            shared_resource_ref(const shared_resource_ref &ref)
            {
                m_res=ref.m_res;
                m_res_holder=ref.m_res_holder;
                m_creator=ref.m_creator;

                ref_count_inc();
            }

            shared_resource_ref &operator=(const shared_resource_ref &ref)
            {
                if(this==&ref)
                    return *this;

                free();

                m_res=ref.m_res;
                m_res_holder=ref.m_res_holder;
                m_creator=ref.m_creator;

                ref_count_inc();

                return *this;
            }

            ~shared_resource_ref() { free(); }

        protected:
            shared_resource_ref(t_res*res,res_holder*holder,shared_resources_creator *creator):
                                m_res(res),m_res_holder(holder),m_creator(creator) {}
        private:
            void ref_count_inc()
            {
                if(m_creator)
                    m_creator->res_ref_count_inc(*this);
            }

        protected:
            t_res *m_res;

        private:
            res_holder *m_res_holder;
            shared_resources_creator *m_creator;
        };

        class shared_resource_mutable_ref: public shared_resource_ref
        {
            template<typename,int> friend class shared_resources;

        public:
            t_res *get() { return this->m_res; }
            t_res *operator -> () { return this->m_res; }

        public:
            shared_resource_mutable_ref() {}

        private:
            shared_resource_mutable_ref(t_res*res,res_holder*holder,shared_resources_creator *creator)
            { *(shared_resource_ref*)this=shared_resource_ref(res,holder,creator); }
        };

    public:
        shared_resource_ref access(const char*name)
        {
            if(!name || !m_base)
                return shared_resource_ref();

            std::string name_str(name);
            if(m_force_lowercase)
                std::transform(name_str.begin(),name_str.end(),name_str.begin(),::tolower);

            std::pair<resources_map_iterator,bool> ir = m_res_map.insert(std::make_pair(name_str,(res_holder*)0));
            if(ir.second)
            {
                res_holder *holder=m_res_pool.allocate();
                if(!holder)
                    return shared_resource_ref();

                ir.first->second = holder;
                if(!m_base->fill_resource(name,holder->res))
                {
                    m_res_map.erase(ir.first);
                    return shared_resource_ref();
                }
                holder->ref_count=1;
                holder->map_it=ir.first;

                ++m_ref_count;

                return shared_resource_ref(&(holder->res),holder,this);
            }
            else
            {
                res_holder *holder=ir.first->second;
                if(holder)
                {
                    ++holder->ref_count;

                    return shared_resource_ref(&(holder->res),holder,this);
                }
            }

            return shared_resource_ref();
        }

        shared_resource_mutable_ref create()
        {
            res_holder *holder=m_res_pool.allocate();
            if(!holder)
                return shared_resource_mutable_ref();

            holder->ref_count=1;
            holder->map_it=m_res_map.end();

            ++m_ref_count;

            return shared_resource_mutable_ref(&(holder->res),holder,this);
        }

        static shared_resource_mutable_ref modify(const shared_resource_ref &ref)
        {
            if(!ref.is_valid())
                return shared_resource_mutable_ref();

            res_ref_count_inc(ref);
            return shared_resource_mutable_ref(&(ref.m_res_holder->res),ref.m_res_holder,ref.m_creator);
        }

        static int res_get_ref_count(const shared_resource_ref&ref)
        {
            if(!ref.m_res_holder)
                return 0;

            return ref.m_res_holder->ref_count;
        }

        int reload_resources()
        {
            if(!m_base)
                return 0;

            int count=0;

            for(resources_map_iterator it=m_res_map.begin();
                it!=m_res_map.end();++it)
            {
                if(!it->second || it->first.empty())
                    continue;

                m_base->release_resource(it->second->res);
                if(m_base->fill_resource(it->first.c_str(),it->second->res))
                    ++count;
            }

            return count;
        }

        bool reload_resource(const char *name)
        {
            if(!name || !m_base)
                return false;

            std::string name_str(name);
            if(m_force_lowercase)
                std::transform(name_str.begin(),name_str.end(),name_str.begin(),::tolower);

            resources_map_iterator it=m_res_map.find(name_str);
            if(it==m_res_map.end() || !it->second)
                return false;

            m_base->release_resource(it->second->res);
            return m_base->fill_resource(it->first.c_str(),it->second->res);
        }

        const char *get_res_name(const shared_resource_ref&ref)
        {
            if(!ref.m_res_holder)
                return 0;

            if(ref.m_creator!=this)
                return 0;

            if(ref.m_res_holder->map_it==m_res_map.end())
                return 0;

            return ref.m_res_holder->map_it->first.c_str();
        }

        void free(shared_resource_ref&ref)
        {
            if(!ref.m_res_holder)
                return;

            if(ref.m_creator!=this)
                return;

            --ref.m_res_holder->ref_count;

            if(ref.m_res_holder->ref_count>0)
                return;

            ref.m_res_holder->ref_count=0;

            if(!m_should_unload_unused)
                return;

            if(m_ref_count>0)
                --m_ref_count;
            else
                nya_log::log()<<"resource system failure\n";

            if(ref.m_res && m_base)
                m_base->release_resource(*ref.m_res);

            if(ref.m_res_holder->map_it!=m_res_map.end())
            {
                if(!m_base)
                    nya_log::log()<<"warning: unreleased resource "<<ref.m_res_holder->map_it->first.c_str()<<"\n";

                m_res_map.erase(ref.m_res_holder->map_it);
            }

            m_res_pool.free(ref.m_res_holder);

            if(!m_ref_count)
            {
                if(!m_base)
                    delete this;
                else
                    nya_log::log()<<"resource system failure\n";
            }
        }

        static void res_ref_count_inc(const shared_resource_ref &ref)
        {
            if(!ref.m_res_holder)
                return;

            ++ref.m_res_holder->ref_count;
        }

        void should_unload_unused(bool unload)
        {
            if(unload && unload!=m_should_unload_unused)
                free_unused();

            m_should_unload_unused=unload;
        }

        void free_unused()
        {
            resources_map_iterator it=m_res_map.begin();
            while(it!=m_res_map.end())
            {
                if(it->second)
                {
                    if(it->second->ref_count>0)
                    {
                        ++it;
                        continue;
                    }

                    if(m_base)
                        m_base->release_resource(it->second->res);

                    m_res_pool.free(it->second);
                }

                resources_map_iterator er = it;
                ++it;

                m_res_map.erase(er);
            }
        }

        void free_all()
        {
            resources_map_iterator it;
            for(it=m_res_map.begin();it!=m_res_map.end();++it)
            {
                if(!it->second)
                    continue;

                if(m_base)
                    m_base->release_resource(it->second->res);

                it->second->map_it=m_res_map.end();
            }

            m_res_map.clear();
            m_res_pool.clear();
        }

        void base_released()
        {
            if(!m_base)
                return;

            m_base=0;

            if(m_ref_count>0)
                --m_ref_count;
        }

        bool has_refs() { return m_ref_count>0; }

    public:
        shared_resources_creator(shared_resources *base): m_base(base),m_should_unload_unused(true),
                                                          m_force_lowercase(true), m_ref_count(1) {}
    private:
        typedef std::map<std::string,res_holder*> resources_map;
        typedef typename resources_map::iterator resources_map_iterator;

    private:
        struct res_holder
        {
            t_res res;
            int ref_count;
            resources_map_iterator map_it;

            res_holder(): ref_count(0) {}
        };

        resources_map m_res_map;
        nya_memory::pool<res_holder,block_count> m_res_pool;

    private:
        shared_resources *m_base;
        bool m_should_unload_unused;
        bool m_force_lowercase;
        size_t m_ref_count;
    };

public:
    typedef typename shared_resources_creator::shared_resource_ref shared_resource_ref;
    typedef typename shared_resources_creator::shared_resource_mutable_ref shared_resource_mutable_ref;

public:
    shared_resource_ref access(const char*name) { return m_creator->access(name); }
    shared_resource_mutable_ref create() { return m_creator->create(); }
    static shared_resource_mutable_ref modify(shared_resource_ref &res) { return shared_resources_creator::modify(res); }

    shared_resources() { m_creator = new shared_resources_creator(this); }

public:
    //void free_all() { m_creator->free_all(); }
    void free_unused() { m_creator->free_unused(); }
    void force_lowercase(bool force) { m_creator->m_force_lowercase=force; }
    void should_unload_unused(bool unload) { m_creator->should_unload_unused(unload); }
    bool reload_resource(const char *name) { return m_creator->reload_resource(name); }
    int reload_resources() { return m_creator->reload_resources(); }

public:
    shared_resource_ref get_first_resource()
    {
        if(m_creator->m_res_map.empty())
            return shared_resource_ref();

        struct shared_resources_creator::res_holder *holder=m_creator->m_res_map.begin()->second;
        if(!holder)
            return shared_resource_ref();

        ++holder->ref_count;
        return shared_resource_ref(&(holder->res),holder,m_creator);
    }

    shared_resource_ref get_next_resource(shared_resource_ref &curr)
    {
        if(curr.m_creator!=m_creator || !curr.m_res_holder)
            return shared_resource_ref();

        typename shared_resources_creator::resources_map_iterator it=curr.m_res_holder->map_it;
        if(++it==m_creator->m_res_map.end())
            return shared_resource_ref();

        struct shared_resources_creator::res_holder *holder=it->second;
        if(!holder)
            return shared_resource_ref();

        ++holder->ref_count;
        return shared_resource_ref(&(holder->res),holder,m_creator);
    }

public:
    virtual ~shared_resources()
    {
        m_creator->base_released();
        if(!m_creator->has_refs())
            delete m_creator;
    }

private:
    class shared_resources_creator *m_creator;
};
}
