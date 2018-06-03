#ifndef TINY_OS_SHARED_BUF_POOL_H
#define TINY_OS_SHARED_BUF_POOL_H

#include <kernel/process/spinlock.h>
#include <shared/freelist.h>

typedef struct
{
    freelist_handle fl;
    spinlock lock;
    size_t unit_size;
} buf_pool_t;

void init_buf_pool(buf_pool_t *pool, size_t unit_size);

void *alloc_from_buf_pool(buf_pool_t *pool);

void free_in_buf_pool(buf_pool_t *pool, void *buf);

#endif /* TINY_OS_SHARED_BUF_POOL_H */
