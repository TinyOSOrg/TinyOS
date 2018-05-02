#include <kernel/filesys/afs/afs.h>
#include <kernel/filesys/afs/memory.h>
#include <kernel/filesys/afs/storage.h>
#include <kernel/memory.h>

#include <shared/freelist.h>

/* 空闲的afs handle自由链表 */
static freelist_handle empty_afs_handle_freelist;

/* 申请一个未初始化的afs handle */
static struct afs_handle *alloc_afs_handle(void)
{
    if(is_freelist_empty(&empty_afs_handle_freelist))
    {
        struct afs_handle *new_handles = (struct afs_handle*)
            alloc_ker_page(true);
        size_t end = 4096 / sizeof(struct afs_handle);
        for(size_t i = 0;i != end; ++i)
            add_freelist(&empty_afs_handle_freelist, &new_handles[i]);
    }
    return fetch_freelist(&empty_afs_handle_freelist);
}

/* 释放一块不再使用的afs handle空间 */
static inline void release_afs_handle_zone(struct afs_handle *handle)
{
    add_freelist(&empty_afs_handle_freelist, handle);
}

void init_afs(void)
{
    init_freelist(&empty_afs_handle_freelist);
}

struct afs_handle *construct_new_afs(const struct dpt_unit *dpt)
{
    ASSERT_S(dpt->type != DISK_PT_NONEXISTENT);
    ASSERT_S(dpt->sector_begin < dpt->sector_end);
    
    //TODO
    return alloc_afs_handle();
}
