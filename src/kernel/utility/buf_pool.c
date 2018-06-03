#include <kernel/assert.h>
#include <kernel/memory.h>
#include <kernel/utility/bufpool.h>

void init_buf_pool(buf_pool_t *pool, size_t unit_size)
{
    ASSERT(pool && unit_size && unit_size < 4096);
    init_freelist(&pool->fl);
    init_spinlock(&pool->lock);
    pool->unit_size = unit_size;
}

void *alloc_from_buf_pool(buf_pool_t *pool)
{
    ASSERT(pool);
    spinlock_lock(&pool->lock);
    if(is_freelist_empty(&pool->fl))
    {
        char *buf = alloc_ker_page(true);
        size_t end = 4096 / pool->unit_size;
        for(size_t i = 0; i < end; ++i)
        {
            add_freelist(&pool->fl, buf);
            buf += pool->unit_size;
        }
    }
    void *ret = fetch_freelist(&pool->fl);
    spinlock_unlock(&pool->lock);
    return ret;
}

void free_in_buf_pool(buf_pool_t *pool, void *buf)
{
    ASSERT(pool && buf);
    spinlock_lock(&pool->lock);
    add_freelist(&pool->fl, buf);
    spinlock_unlock(&pool->lock);
}
