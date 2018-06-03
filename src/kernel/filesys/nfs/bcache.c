#include <kernel/driver/diskdriver.h>
#include <kernel/filesys/nfs/bcache.h>
#include <kernel/memory.h>
#include <kernel/process/spinlock.h>
#include <kernel/utility/bufpool.h>

#include <shared/freelist.h>
#include <shared/ptrlist.h>
#include <shared/rbtree.h>

typedef struct
{
    uint32_t sec;

    // linked map
    struct rb_node    tn;
    struct ilist_node in;

    unsigned int rlock : 14; // 读计数
    unsigned int wlock : 1;  // 写入锁
    unsigned int dflag : 1;  // 删除标志

    void *data;
} bnode_t;

/* bnode和block的池子 */
static buf_pool_t bnode_pool;
static buf_pool_t blk_pool;

/* bnode linked map */
static struct rb_tree bnode_tree;
static ilist          bnode_list;
static size_t         bnode_count;

void nfs_init_bcache()
{
    init_buf_pool(&bnode_pool, sizeof(bnode_t));
    init_buf_pool(&blk_pool, NFS_BLK_SIZE);

    rb_init(&bnode_tree);
    init_ilist(&bnode_list);
    bnode_count = 0;
}

/* 申请一块block缓存 */
void *nfs_balloc()
{
    return alloc_from_buf_pool(&blk_pool);
}

/* 释放一块block缓存 */
void nfs_bfree(void *buf)
{
    free_in_buf_pool(&blk_pool, buf);
}

/* 开始读取磁盘上的block */
void nfs_rb_beg(uint32_t b)
{
    // TODO
}

/* 结束读取磁盘上的block */
void nfs_rb_end(uint32_t b);

/* 开始读写磁盘上的block */
void nfs_wb_beg(uint32_t b);

/* 结束读写磁盘上的block */
void nfs_rb_end(uint32_t b);
