#ifndef TINY_OS_LIB_MEM_H
#define TINY_OS_LIB_MEM_H

#include <shared/stdint.h>

static inline void *malloc(size_t size)
{
    extern void *_malloc(size_t);
    return _malloc(size);
}

static inline void free(void *ptr)
{
    extern void _free(void*);
    _free(ptr);
}

#endif /* TINY_OS_LIB_MEM_H */
