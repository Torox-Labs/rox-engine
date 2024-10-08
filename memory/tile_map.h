//nya-engine (C) nyan.developer@gmail.com released under the MIT license (see LICENSE)

#pragma once

#include <vector>

namespace nya_memory
{

template <class index=int,int invalid=-1>
class tile_map
{
public:
    index get(int x,int y) const
    {
        x-=m_x,y-=m_y;
        if(x<0 || y<0 || x>=m_w || y>=m_h)
            return index(invalid);

        return m_tiles[y*m_w+x];
    }

    void set(index i,int x,int y)
    {
        if(x<m_x || y<m_y || x>=m_x+m_w || y>=m_y+m_h)
        {
            const int new_x=x<m_x?x:m_x;
            const int new_y=y<m_y?y:m_y;

            int new_w=x-new_x+1;
            if(new_w<m_w+m_x-new_x)
                new_w=m_w+m_x-new_x;

            int new_h=y-new_y+1;
            if(new_h<m_h+m_y-new_y)
                new_h=m_h+m_y-new_y;

            resize(new_x,new_y,new_w,new_h);
        }

        x-=m_x,y-=m_y;
        m_tiles[y*m_w+x]=i;
    }

    void set(int x,int y,int w,int h,const index *indices)
    {
        if(!indices)
            return;

        resize(x,y,w,h);
        memcpy(m_tiles.data(),indices,w*h*sizeof(index));
    }

    void resize(int x,int y,int w,int h)
    {
        if(x==m_x && y==m_y && w==m_w && m_h==h)
            return;

        if(!m_tiles.empty())
        {
            //ToDo: inplace when possible
            std::vector<index> old_tiles=m_tiles;

            m_tiles.clear();
            m_tiles.resize(w*h,index(invalid));

            int y0=0,y1=m_y-y;
            if(y1<0)
                y0-=y1,y1=0;

            for(;y0<m_h && y1<h;++y0,++y1)
            {
                int x0=0,x1=m_x-x;
                if(x1<0)
                    x0-=x1,x1=0;

                for(;x0<m_w && x1<w;++x0,++x1)
                    m_tiles[w*y1+x1]=old_tiles[m_w*y0+x0];
            }
        }
        else
            m_tiles.resize(w*h,index(invalid));

        m_x=x,m_y=y;
        m_w=w,m_h=h;
    }

    void clear() { *this=tile_map(); }

    int get_x() const { return m_x; }
    int get_y() const { return m_y; }
    int get_w() const { return m_w; }
    int get_h() const { return m_h; }

    const index *get_tiles() const { return m_tiles.data(); }

public:
    tile_map(): m_x(0),m_y(0),m_w(0),m_h(0) {}

private:
    int m_x,m_y,m_h,m_w;
    std::vector<index> m_tiles;
};

}
