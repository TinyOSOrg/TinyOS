#include <kernel/filesys/afs/blk_mem_buf.h>
#include <kernel/filesys/afs/dp_phy.h>
#include <kernel/interrupt.h>
#include <kernel/memory.h>

#include <shared/freelist.h>

static freelist_handle blk_buf_fl;

static freelist_handle sec_buf_fl;

void init_afs_buffer_allocator(void)
{
    init_freelist(&blk_buf_fl);
    init_freelist(&sec_buf_fl);
}

STATIC_ASSERT(AFS_BLOCK_BYTE_SIZE <= 4096,
              too_large_afs_block_size);

/*
    这个函数可能被多个进程并行调用，而那些进程很可能是不关中断的
    所以这里的alloc和free都要关中断操作
*/
void *afs_alloc_block_buffer(void)
{    
    intr_state is = fetch_and_disable_intr();

    if(is_freelist_empty(&blk_buf_fl))
    {
        char *page = (char*)alloc_ker_page(true);
        for(size_t i = 0; i < 4096 / AFS_BLOCK_BYTE_SIZE; ++i)
            add_freelist(&blk_buf_fl, page + AFS_BLOCK_BYTE_SIZE * i);
    }

    void *ret = fetch_freelist(&blk_buf_fl);
    set_intr_state(is);
    return ret;
}

void afs_free_block_buffer(void *buf)
{
    intr_state is = fetch_and_disable_intr();
    add_freelist(&blk_buf_fl, buf);
    set_intr_state(is);
}

void *afs_alloc_sector_buffer(void)
{
    intr_state is = fetch_and_disable_intr();

    if(is_freelist_empty(&sec_buf_fl))
    {
        char *block = (char*)afs_alloc_block_buffer();
        for(size_t i = 0; i < AFS_BLOCK_SECTOR_COUNT; ++i)
            add_freelist(&sec_buf_fl, block + AFS_SECTOR_BYTE_SIZE * i);
    }

    void *ret = fetch_freelist(&sec_buf_fl);
    set_intr_state(is);
    return ret;
}

void afs_free_sector_buffer(void *buf)
{
    intr_state is = fetch_and_disable_intr();
    add_freelist(&sec_buf_fl, buf);
    set_intr_state(is);
}
