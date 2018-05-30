#ifndef TINY_OS_LIB_MEM_H
#define TINY_OS_LIB_MEM_H

#include <shared/stdint.h>

#define malloc(size) \
    ({ \
        extern void *_malloc(size_t); \
        _malloc(size); \
    })

#define free(ptr) \
    ({ \
        extern void _free(void*); \
        _free(ptr); \
    })

#endif /* TINY_OS_LIB_MEM_H */
