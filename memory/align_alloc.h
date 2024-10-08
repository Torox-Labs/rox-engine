//nya-engine (C) nyan.developer@gmail.com released under the MIT license (see LICENSE)

#pragma once

#ifdef _MSC_VER
    #include <malloc.h>
#else
    #include <stdlib.h>
#endif

namespace nya_memory
{

inline void* align_alloc(size_t size,size_t align)
{
#ifdef _MSC_VER
    return _aligned_malloc(size,align);
#elif __MINGW32__
    return __mingw_aligned_malloc(size,align);
#elif __ANDROID__
    return memalign(align,size);
#else
    void *result=0;
    return posix_memalign(&result,align,size)==0 ? result:0;
#endif
}

inline void align_free(void *p)
{
#ifdef _MSC_VER
    _aligned_free(p);
#elif __MINGW32__
    __mingw_aligned_free(p);
#else
    free(p);
#endif
}

template<typename t> t *align_new(size_t align)
{
    void *p=align_alloc(sizeof(t),align);
    return p?new(p)t():0;
}

template<typename t> t *align_new(const t &from,size_t align)
{
    void *p=align_alloc(sizeof(t),align);
    return p?new(p)t(from):0;
}

template<typename t> void align_delete(t *p)
{
    p->~t();
    align_free(p);
}

template <typename t,size_t align> struct aligned_allocator
{
    typedef t value_type;
    template<class u> struct rebind { typedef aligned_allocator<u,align> other; };

    static t* allocate(size_t n) { t *r=(t*)align_alloc(sizeof(t)*n,align);  for(size_t i=0;i<n;++i) new(r+i)t(); return r; }
    static void deallocate(t* ptr,size_t n) { for(size_t i=0;i<n;++i) (ptr+i)->~t(); nya_memory::align_free(ptr); }

    aligned_allocator() {}
    template<typename a> aligned_allocator(a &) {}
};

inline bool is_aligned(const void *ptr,size_t align) { return (size_t)ptr % align == 0; }

}
