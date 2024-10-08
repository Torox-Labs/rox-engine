//nya-engine (C) nyan.developer@gmail.com released under the MIT license (see LICENSE)

#pragma once

namespace nya_memory
{

template<typename t> class optional
{
public:
    bool is_valid() const { return m_obj!=0; }

    const t *operator -> () const { return m_obj; };
    t *operator -> () { return m_obj; };

    void allocate()
    {
        if(m_obj)
            return;

        m_obj=new t;
    }

    void free()
    {
        if(!m_obj)
            return;

        delete m_obj;
        m_obj=0;
    }

    optional():m_obj(0) {}

    optional(const optional &from)
    {
        if(from.m_obj)
        {
            m_obj=new t;
            *m_obj=*from.m_obj;
        }
        else
            m_obj=0;
    }

    ~optional() { free(); }

    optional &operator = (const optional &from)
    {
        if(this==&from)
            return *this;

        if(from.m_obj)
        {
            allocate();
            *m_obj=*from.m_obj;
        }
        else
            free();

        return *this;
    }

private:
    t* m_obj;
};

}
