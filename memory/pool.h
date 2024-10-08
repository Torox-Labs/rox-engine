//nya-engine (C) nyan.developer@gmail.com released under the MIT license (see LICENSE)

#pragma once

#include "non_copyable.h"
#include <vector>

namespace nya_memory
{

template<typename t_data,size_t block_elements_count> class pool: public non_copyable
{
public:
    t_data *allocate()
    {
        size_t free_block_idx, free_offset;

        if(m_free_node_idx==no_idx)
        {
            block *b=new block();

            m_free_node_idx=block_elements_count*m_blocks.size();
            size_t next_idx=m_free_node_idx+1;

            for(size_t i=0;i<block_elements_count;++i)
            {
                node &n=b->nodes[i];
                n.block_idx=no_idx;
                n.next_free=next_idx;
                ++next_idx;
            }

            b->nodes[block_elements_count-1].next_free=no_idx;

            m_blocks.push_back(b);
        }

        free_block_idx=m_free_node_idx / block_elements_count;
        free_offset=m_free_node_idx % block_elements_count;

        node &n=m_blocks[free_block_idx]->nodes[free_offset];
        m_free_node_idx=n.next_free;

        n.block_idx=free_block_idx;
        n.next_free=no_idx;

        ++m_used_count;

        new (n.data) t_data;
        return (t_data*)n.data;
    }

    bool free(const t_data *data )
    {
        if(!data)
            return false;

        node &n = *((node *)(((char *)data) - offsetof(node, data)));

        if(n.block_idx>=m_blocks.size() || &n < &(m_blocks[n.block_idx]->nodes[0]))
            return false;

        const size_t idx=(size_t)(&n - &(m_blocks[n.block_idx]->nodes[0]));
        if(idx>=block_elements_count)
            return false;

        data->~t_data();

        n.next_free=m_free_node_idx;
        m_free_node_idx=n.block_idx*block_elements_count + idx;
        n.block_idx=no_idx;

        --m_used_count;

        return true;
     }

    void clear()
    {
        m_free_node_idx=m_blocks.empty()?no_idx:0;

        size_t next_node_idx=1;

        for (size_t i=0;i<m_blocks.size();++i)
        {
            for(size_t j=0;j<block_elements_count;++j)
            {
                node & n=m_blocks[i]->nodes[j];

                if(n.block_idx!=no_idx)
                    ((t_data*)n.data)->~t_data();

                n.block_idx=no_idx;
                n.next_free=next_node_idx;

                ++next_node_idx;
            }
        }

        if(!m_blocks.empty())
            m_blocks.back()->nodes[block_elements_count-1].next_free=no_idx;

        m_used_count = 0;
    }

public:
    size_t get_count() const { return m_used_count; }
    size_t get_mem_size() const { return m_blocks.size()*sizeof(t_data)*block_elements_count; }

public:
    pool(): m_free_node_idx(no_idx),m_used_count(0) {}
    ~pool() { for(size_t i=0;i<m_blocks.size();++i) delete m_blocks[i]; }

private:
    struct node
    {
        size_t block_idx;
        size_t next_free;
        char data[sizeof(t_data)];
    };

    struct block { node nodes[block_elements_count]; };

    size_t m_free_node_idx;
    size_t m_used_count;

    std::vector<block*> m_blocks;

    static const size_t no_idx=(size_t)-1;
};

}
