#include <kernel/assert.h>
#include <kernel/diskdriver.h>
#include <kernel/interrupt.h>
#include <kernel/memory.h>
#include <kernel/process/spinlock.h>

#include <kernel/filesys/afs/blk_mem_buf.h>
#include <kernel/filesys/afs/dp_phy.h>
#include <kernel/filesys/afs/disk_cache.h>

#include <shared/freelist.h>
#include <shared/ptrlist.h>
#include <shared/rbtree.h>
#include <shared/string.h>
#include <shared/utility.h>

//=========================== 扇区缓存管理 ===========================

struct sector_node
{
    // linked map结构
    struct ilist_node list_node;
    struct rb_node    tree_node;
    // 扇区号
    uint32_t sec;
    // buffer本身
    void *buffer;
    // 读者计数器
    unsigned int reader_count : 13;
    // 写者锁
    unsigned int writer_lock  : 1;
    // 销毁标志
    unsigned int release_flag : 1;
    // 脏位
    unsigned int dirty        : 1;
};

/* 空闲sector_node的自由链表 */
static freelist_handle empty_sector_nodes;
/* empty_sector_nodes的锁 */
static spinlock empty_sector_nodes_lock;

/*
    分配一个空的sector_node
    这个函数以及下面的free函数都可能在多个不同进程中被调用
    而操作时间又很短，所以spinlock
*/
static struct sector_node *alloc_sector_node()
{
    spinlock_lock(&empty_sector_nodes_lock);

    if(is_freelist_empty(&empty_sector_nodes))
    {
        struct sector_node *nodes = (struct sector_node*)alloc_ker_page(true);
        size_t end = 4096 / sizeof(struct sector_node);
        for(size_t i = 0;i < end; ++i)
            add_freelist(&empty_sector_nodes, &nodes[i]);
    }

    struct sector_node *ret = (struct sector_node*)fetch_freelist(&empty_sector_nodes);
    spinlock_unlock(&empty_sector_nodes_lock);
    return ret;
}

static void free_sector_node(struct sector_node *node)
{
    spinlock_lock(&empty_sector_nodes_lock);
    add_freelist(&empty_sector_nodes, node);
    spinlock_unlock(&empty_sector_nodes_lock);
}

/*
    扇区缓存记录，linked map，节点类型为sector_node
    list的back为最新的，front为最旧的
*/
static ilist sec_list;
static size_t sec_list_size;
static struct rb_tree sec_tree;

#define KOF MEM_TO_MEM_OFFSET(struct sector_node, tree_node, sec)

static bool rb_less(const void *L, const void *R)
{
    return *(uint32_t*)L < *(uint32_t*)R;
}

/* 释放一块sec缓存，如果它是脏的，就执行写回操作 */
static void release_sec_buf(struct sector_node *node)
{
    // afs_write_sector_raw会调度其他线程，所以要先保证list和tree中的node已经被删了
    erase_from_ilist(&node->list_node);
    rb_erase(&sec_tree, &node->tree_node, KOF, rb_less);
    --sec_list_size;

    if(node->dirty)
        afs_write_sector_raw(node->sec, node->buffer);

    afs_free_sector_buffer(node->buffer);
    free_sector_node(node);
}

#define MAX_SEC_LRU_SIZE 32

/*
    尝试释放超出数量阈值的、最近没怎么用过的sec缓存
    如果发现有人正在使用，就置release标志为1，否则直接释放掉
*/
static void try_release_redundant_secs()
{
RESTART:

    if(sec_list_size <= MAX_SEC_LRU_SIZE)
        return;

    // 计算需要尝试释放的节点数量
    size_t try_cnt = sec_list_size - MAX_SEC_LRU_SIZE;

    struct ilist_node *in = sec_list.next, *next = NULL;
    for(size_t i = 0;i != try_cnt; ++i, in = next)
    {
        ASSERT_S(in != &sec_list);

        // in所在空间稍后可能被释放，所以这里提前取得下一个节点的地址
        next = in->next;

        struct sector_node *node = GET_STRUCT_FROM_MEMBER(
            struct sector_node, list_node, in);
        if(node->reader_count || node->writer_lock)
            node->release_flag = 1;
        else
        {
            release_sec_buf(node);

            // IMPROVE
            // release_sec_buf涉及到其他线程的调度，可能导致目前对list的遍历失效
            // 所以一旦真正释放了一个块，就重启对list的遍历
            // 的确慢了点，以后想办法优化
            goto RESTART;
        }
    }
}

/* 开始读一个扇区
    在红黑树中查找目标
        找到了，有人在写，yieldCPU，等会儿重新查找
               无人在写，读计数器++，开始用
        没找到，从磁盘读进来加入LRU管理，然后淘汰多出来的缓存
*/
static void *afs_read_sector_entry(uint32_t sec)
{
    intr_state is = fetch_and_disable_intr();
    void *ret = NULL;
    struct rb_node *rbn = NULL;

RESTART:

    // 在红黑树中查找该扇区缓存
    rbn = rb_find(&sec_tree, KOF, &sec, rb_less);

    if(rbn)
    {
        struct sector_node *node = GET_STRUCT_FROM_MEMBER(
            struct sector_node, tree_node, rbn);

        // 更新LRU，让这块缓存别被释放了
        erase_from_ilist(&node->list_node);
        push_back_ilist(&sec_list, &node->list_node);
        node->release_flag = 0;

        // buffer为NULL说明有别的线程创建了这个扇区记录，正在读磁盘，故也应该让出CPU
        if(node->writer_lock || !node->buffer)
        {
            yield_CPU();
            goto RESTART;
        }
        else
        {
            node->reader_count++;
            ret = node->buffer;
        }
    }
    else
    {
        // 注意到这里先把nn->buffer置为空
        // 等到相关数据结构(list, tree, node)都更新完成了才真正从磁盘读数据
        // 这是因为读磁盘时会调度其他线程，必须保证数据结构完整性

        struct sector_node *nn = alloc_sector_node();
        nn->sec          = sec;
        nn->buffer       = NULL;
        nn->reader_count = 1;
        nn->writer_lock  = 0;
        nn->release_flag = 0;
        nn->dirty        = 0;

        rb_insert(&sec_tree, &nn->tree_node, KOF, rb_less);
        push_back_ilist(&sec_list, &nn->list_node);
        ++sec_list_size;

        void *nb = afs_alloc_sector_buffer();
        afs_read_sector_raw(sec, nb);
        nn->buffer = nb;
        ret = nb;
    }

    try_release_redundant_secs();
    set_intr_state(is);

    return ret;
}

static void afs_read_sector_exit(uint32_t sec)
{
    intr_state is = fetch_and_disable_intr();

    // 一定能在红黑树中找到这个扇区的缓存
    struct rb_node *rbn = rb_find(&sec_tree, KOF, &sec, rb_less);
    ASSERT_S(rbn != NULL);
    struct sector_node *node = GET_STRUCT_FROM_MEMBER(
            struct sector_node, tree_node, rbn);        
    ASSERT_S(node->reader_count);

    // 如果自己是最后一个读者，查看是否需要将这块缓存释放
    node->reader_count--;
    if(!node->reader_count && node->release_flag)
        release_sec_buf(node);

    set_intr_state(is);
}

void afs_read_from_sector(uint32_t sec, size_t offset, size_t size, void *data)
{
    ASSERT_S(size > 0 && offset + size <= AFS_SECTOR_BYTE_SIZE);

    void *buf = afs_read_sector_entry(sec);

    memcpy((char*)data, (char*)buf + offset, size);

    afs_read_sector_exit(sec);
}

/* 开始写一个扇区
    在红黑树中查找目标
        找到了，有人在读/写，yieldCPU等会儿再试
               没人用，置写标志位，开始使用
        没找到，从磁盘读进来加入LRU管理，开始使用
*/
static void *afs_write_sector_entry(uint32_t sec)
{
    intr_state is = fetch_and_disable_intr();
    void *ret = NULL;
    struct rb_node *rbn = NULL;

RESTART:

    rbn = rb_find(&sec_tree, KOF, &sec, rb_less);

    if(rbn)
    {
        struct sector_node *node = GET_STRUCT_FROM_MEMBER(
            struct sector_node, tree_node, rbn);

        // 更新LRU
        erase_from_ilist(&node->list_node);
        push_back_ilist(&sec_list, &node->list_node);
        node->release_flag = 0;

        if(node->reader_count || node->writer_lock || !node->buffer)
        {
            yield_CPU();
            goto RESTART;
        }
        else
        {
            node->writer_lock  = 1;
            node->dirty        = 1;
            ret = node->buffer;
        }
    }
    else
    {
       struct sector_node *nn = alloc_sector_node();
       nn->sec          = sec;
       nn->buffer       = NULL;
       nn->reader_count = 0;
       nn->writer_lock  = 1;
       nn->release_flag = 0;
       nn->dirty        = 1;

       rb_insert(&sec_tree, &nn->tree_node, KOF, rb_less);
       push_back_ilist(&sec_list, &nn->list_node);
       ++sec_list_size;

       void *nb = afs_alloc_sector_buffer();
       afs_read_sector_raw(sec, nb);
       nn->buffer = nb;
       ret = nb;
    }

    try_release_redundant_secs();
    set_intr_state(is);
    return ret;
}

static void afs_write_sector_exit(uint32_t sec)
{
    intr_state is = fetch_and_disable_intr();

    // 一定能在红黑树中找到这个扇区的缓存
    struct rb_node *rbn = rb_find(&sec_tree, KOF, &sec, rb_less);
    ASSERT_S(rbn != NULL);
    struct sector_node *node = GET_STRUCT_FROM_MEMBER(
            struct sector_node, tree_node, rbn);
    ASSERT_S(node->writer_lock && !node->reader_count);

    // 自己肯定是唯一的操作者，查看这块缓存的释放标志
    node->writer_lock = 0;
    if(node->release_flag)
        release_sec_buf(node);

    set_intr_state(is);
}

void afs_write_to_sector(uint32_t sec, size_t offset, size_t size, const void *data)
{
   ASSERT_S(size > 0 && offset + size <= AFS_SECTOR_BYTE_SIZE);

   void *buf = afs_write_sector_entry(sec);

   memcpy((char*)buf + offset, (char*)data, size);

   afs_write_sector_exit(sec);
}

void afs_release_all_sector_cache()
{
    intr_state is = fetch_and_disable_intr();

    while(!is_ilist_empty(&sec_list))
    {
        struct ilist_node *in = sec_list.next;
        release_sec_buf(GET_STRUCT_FROM_MEMBER(
            struct sector_node, list_node, in));
    }
    sec_list_size = 0;

    set_intr_state(is);
}

const void *afs_read_from_sector_begin(uint32_t sec)
{
    return afs_read_sector_entry(sec);
}

void afs_read_from_sector_end(uint32_t sec)
{
    afs_read_sector_exit(sec);
}

void *afs_write_to_sector_begin(uint32_t sec)
{
    return afs_write_sector_entry(sec);
}

void afs_write_to_sector_end(uint32_t sec)
{
    afs_write_sector_exit(sec);
}

#undef KOF

//=========================== block缓存管理 ===========================

struct blk_node
{
    // linked map 结构
    struct ilist_node list_node;
    struct rb_node    tree_node;
    // 扇区号
    uint32_t sec;
    // buffer本身
    void *buffer;
    // 使用者计数器。block缓存管理并不负责读写互斥
    unsigned int user_count   : 14;
    // 销毁标志
    unsigned int release_flag : 1;
    // 脏位
    unsigned int dirty        : 1;
};

static freelist_handle empty_blk_nodes;
static spinlock empty_blk_nodes_lock;

static struct blk_node *alloc_blk_node()
{
    spinlock_lock(&empty_blk_nodes_lock);

    if(is_freelist_empty(&empty_blk_nodes))
    {
        struct blk_node *nodes = (struct blk_node*)alloc_ker_page(true);
        size_t end = 4096 / sizeof(struct blk_node);
        for(size_t i = 0;i < end; ++i)
            add_freelist(&empty_blk_nodes, &nodes[i]);
    }

    struct blk_node *ret = (struct blk_node*)fetch_freelist(&empty_blk_nodes);
    spinlock_unlock(&empty_blk_nodes_lock);
    return ret;
}

static void free_blk_node(struct blk_node *node)
{
    spinlock_lock(&empty_blk_nodes_lock);
    add_freelist(&empty_blk_nodes, node);
    spinlock_unlock(&empty_blk_nodes_lock);
}

static ilist blk_list;
static size_t blk_list_size;
static struct rb_tree blk_tree;

#define KOF MEM_TO_MEM_OFFSET(struct blk_node, tree_node, sec)

// rb_less用和扇区管理相同的函数

/* 释放一块block，若它是脏的，就执行写回操作 */
static void release_block_buf(struct blk_node *node)
{
    erase_from_ilist(&node->list_node);
    rb_erase(&blk_tree, &node->tree_node, KOF, rb_less);
    --blk_list_size;

    if(node->dirty)
        afs_write_block_raw(node->sec, node->buffer);
    
    afs_free_sector_buffer(node->buffer);
    free_blk_node(node);
}

#define MAX_BLK_LRU_SIZE 32

/*
    尝试释放超出数量阈值的、最近没怎么用过的blk缓存
    如果发现有人正在使用，就置release标志为1，否则直接释放掉
*/
static void try_release_redundant_blks()
{
RESTART:

    if(blk_list_size <= MAX_BLK_LRU_SIZE)
        return;
    size_t try_cnt = blk_list_size - MAX_BLK_LRU_SIZE;

    struct ilist_node *in = blk_list.next, *next = NULL;
    for(size_t i = 0;i != try_cnt; ++i, in = next)
    {
        ASSERT_S(in != &blk_list);
        next = in->next;

        struct blk_node *node = GET_STRUCT_FROM_MEMBER(
            struct blk_node, list_node, in);
        if(node->user_count)
            node->release_flag = 1;
        else
        {
            release_block_buf(node);
            goto RESTART;
        }
    }
}

/*
    开始访问一个block
    逻辑同读扇区
*/
static void *afs_access_block_entry(uint32_t sec, bool dirty)
{
    intr_state is = fetch_and_disable_intr();
    void *ret = NULL;
    struct rb_node *rbn = NULL;

RESTART:

    if((rbn = rb_find(&blk_tree, KOF, &sec, rb_less)))
    {
        struct blk_node *node = GET_STRUCT_FROM_MEMBER(
            struct blk_node, tree_node, rbn);
        
        erase_from_ilist(&node->list_node);
        push_back_ilist(&blk_list, &node->list_node);
        node->release_flag = 0;

        if(dirty)
            node->dirty = 1;

        if(!node->buffer)
        {
            yield_CPU();
            goto RESTART;
        }
        else
        {
            node->user_count++;
            ret = node->buffer;
        }
    }
    else
    {
        struct blk_node *nn = alloc_blk_node();
        nn->sec          = sec;
        nn->buffer       = NULL;
        nn->user_count   = 1;
        nn->release_flag = 0;
        nn->dirty        = dirty ? 1 : 0;

        rb_insert(&blk_tree, &nn->tree_node, KOF, rb_less);
        push_back_ilist(&blk_list, &nn->list_node);
        ++blk_list_size;

        void *nb = afs_alloc_block_buffer();
        afs_read_block_raw(sec, nb);
        nn->buffer = nb;
        ret = nb;
    }

    try_release_redundant_blks();
    set_intr_state(is);

    return ret;
}

static void afs_access_block_exit(uint32_t sec)
{
    intr_state is = fetch_and_disable_intr();

    struct rb_node *rbn = rb_find(&blk_tree, KOF, &sec, rb_less);
    ASSERT_S(rbn != NULL);
    struct blk_node *node = GET_STRUCT_FROM_MEMBER(
            struct blk_node, tree_node, rbn);
    ASSERT_S(node->user_count);

    node->user_count--;
    if(!node->user_count && node->release_flag)
        release_block_buf(node);
    
    set_intr_state(is);
}

void afs_read_from_block(uint32_t sec, size_t offset, size_t size, void *data)
{
    ASSERT_S(size > 0 && offset + size <= AFS_BLOCK_BYTE_SIZE);

    void *buf = afs_access_block_entry(sec, false);

    memcpy((char*)data, (char*)buf + offset, size);

    afs_access_block_exit(sec);
}

void afs_write_to_block(uint32_t sec, size_t offset, size_t size, const void *data)
{
    ASSERT_S(size > 0 && offset + size <= AFS_BLOCK_BYTE_SIZE);

    void *buf = afs_access_block_entry(sec, true);

    memcpy((char*)buf + offset, (char*)data, size);

    afs_access_block_exit(sec);
}

void afs_release_all_block_cache()
{
    intr_state is = fetch_and_disable_intr();

    while(!is_ilist_empty(&blk_list))
    {
        struct ilist_node *in = blk_list.next;
        release_block_buf(GET_STRUCT_FROM_MEMBER(
            struct blk_node, list_node, in));
    }
    blk_list_size = 0;

    set_intr_state(is);
}

const void *afs_read_from_block_begin(uint32_t sec)
{
    return afs_access_block_entry(sec, false);
}

void afs_read_from_block_end(uint32_t sec)
{
    afs_access_block_exit(sec);
}

void *afs_write_to_block_begin(uint32_t sec)
{
    return afs_access_block_entry(sec, true);
}

void afs_write_to_block_end(uint32_t sec)
{
    afs_access_block_exit(sec);
}

#undef KOF

//=========================== 其他 ===========================

void init_afs_disk_cache()
{
    init_ilist(&sec_list);
    sec_list_size = 0;
    rb_init(&sec_tree);

    init_ilist(&blk_list);
    blk_list_size = 0;
    rb_init(&blk_tree);

    init_freelist(&empty_sector_nodes);
    init_spinlock(&empty_sector_nodes_lock);
}
