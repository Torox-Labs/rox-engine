//nya-engine (C) nyan.developer@gmail.com released under the MIT license (see LICENSE)

#pragma once

#ifdef _MSC_VER
    #include <malloc.h>
#else
    #include <stdlib.h>
#endif

namespace RoxMemory
{

inline void* alignAlloc(size_t size,size_t align)
{
#ifdef _MSC_VER
    return _aligned_malloc(size,align);
#elif __MINGW32__
    return __mingw_aligned_malloc(size,align);
#else
    void *result=0;
    return posix_memalign(&result,align,size)==0 ? result:0;
#endif
}

inline void alignFree(void *p)
{
#ifdef _MSC_VER
    _aligned_free(p);
#elif __MINGW32__
    __mingw_aligned_free(p);
#else
    free(p);
#endif
}

template<typename t> t *alignNew(size_t align)
{
    void *p= alignAlloc(sizeof(t),align);
    return p?new(p)t():0;
}

template<typename t> t *alignNew(const t &from,size_t align)
{
    void *p= alignAlloc(sizeof(t),align);
    return p?new(p)t(from):0;
}

template<typename t> void alignDelete(t *p)
{
    p->~t();
    alignFree(p);
}

template <typename t,size_t align> struct AlignedAllocator
{
    typedef t value_type;
    template<class u> struct rebind { typedef AlignedAllocator<u,align> other; };

    static t* allocate(size_t n) { t *r=(t*)alignAlloc(sizeof(t)*n,align);  for(size_t i=0;i<n;++i) new(r+i)t(); return r; }
    static void deallocate(t* ptr,size_t n) { for(size_t i=0;i<n;++i) (ptr+i)->~t(); RoxMemory::alignFree(ptr); }

    AlignedAllocator() {}
    template<typename a> AlignedAllocator(a &) {}
};

inline bool isAligned(const void *ptr,size_t align) { return (size_t)ptr % align == 0; }

}
