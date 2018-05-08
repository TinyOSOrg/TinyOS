#include <kernel/assert.h>
#include <kernel/diskdriver.h>
#include <kernel/interrupt.h>
#include <kernel/memory.h>
#include <kernel/process/spinlock.h>

#include <kernel/filesys/afs/blk_mem_buf.h>
#include <kernel/filesys/afs/dp_phy.h>
#include <kernel/filesys/afs/sector_cache.h>

#include <shared/freelist.h>
#include <shared/ptrlist.h>
#include <shared/rbtree.h>
#include <shared/string.h>

struct LRUnode
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

/*
    扇区缓存记录，linked map，节点类型为LRUnode
    list的back为最新的，front为最旧的
*/
static ilist sec_list;
static size_t sec_list_size;
static struct rb_tree sec_tree;

/* 空闲LRUnode的自由链表 */
static freelist_handle empty_LRUnodes;
/* empty_LRUnodes的锁 */
static spinlock empty_LRUnodes_lock;

/*
    分配一个空的LRUnode
    这个函数以及下面的free函数都可能在多个不同进程中被调用
    而操作时间又很短，所以spinlock
*/
static struct LRUnode *alloc_LRUnode(void)
{
    spinlock_lock(&empty_LRUnodes_lock);

    if(is_freelist_empty(&empty_LRUnodes))
    {
        struct LRUnode *nodes = (struct LRUnode*)alloc_ker_page(true);
        size_t end = 4096 / sizeof(struct LRUnode);
        for(size_t i = 0;i < end; ++i)
            add_freelist(&empty_LRUnodes, &nodes[i]);
    }

    struct LRUnode *ret = (struct LRUnode*)fetch_freelist(&empty_LRUnodes);
    spinlock_unlock(&empty_LRUnodes_lock);
    return ret;
}

static void free_LRUnode(struct LRUnode *node)
{
    spinlock_lock(&empty_LRUnodes_lock);
    add_freelist(&empty_LRUnodes, node);
    spinlock_unlock(&empty_LRUnodes_lock);
}

void init_afs_sector_cache(void)
{
    init_ilist(&sec_list);
    sec_list_size = 0;
    rb_init(&sec_tree);

    init_freelist(&empty_LRUnodes);
    init_spinlock(&empty_LRUnodes_lock);
}

#define KOF RB_MEM_TO_MEM_OFFSET(struct LRUnode, tree_node, sec)

static bool rb_less(const void *L, const void *R)
{
    return *(uint32_t*)L < *(uint32_t*)R;
}

/* 释放一块sec缓存，如果它是脏的，就执行写回操作 */
static void release_sec_buf(struct LRUnode *node)
{
    // disk_write会调度其他线程，所以要先保证list和tree中的node已经被删了
    erase_from_ilist(&node->list_node);
    rb_erase(&sec_tree, &node->tree_node, KOF, rb_less);
    --sec_list_size;

    if(node->dirty)
        disk_write(node->sec, 1, node->buffer);

    afs_free_sector_buffer(node->buffer);
    free_LRUnode(node);
}

#define MAX_SEC_LRU_SIZE 32

/*
    尝试释放超出数量阈值的、最近没怎么用过的sec缓存
    如果发现有人正在使用，就置release标志为1，否则直接释放掉
*/
static void try_release_redundant_secs(void)
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

        struct LRUnode *node = GET_STRUCT_FROM_MEMBER(
            struct LRUnode, list_node, in);
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
        struct LRUnode *node = GET_STRUCT_FROM_MEMBER(
            struct LRUnode, tree_node, rbn);

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

        struct LRUnode *nn = alloc_LRUnode();
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
        disk_read(sec, 1, nb);
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
    struct LRUnode *node = GET_STRUCT_FROM_MEMBER(
            struct LRUnode, tree_node, rbn);        
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
        struct LRUnode *node = GET_STRUCT_FROM_MEMBER(
            struct LRUnode, tree_node, rbn);

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
       struct LRUnode *nn = alloc_LRUnode();
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
       disk_read(sec, 1, nb);
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
    struct LRUnode *node = GET_STRUCT_FROM_MEMBER(
            struct LRUnode, tree_node, rbn);
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

void afs_release_all_sector_cache(void)
{
    intr_state is = fetch_and_disable_intr();

    while(!is_ilist_empty(&sec_list))
    {
        struct ilist_node *in = pop_front_ilist(&sec_list);
        release_sec_buf(GET_STRUCT_FROM_MEMBER(
            struct LRUnode, list_node, in));
    }
    sec_list_size = 0;

    set_intr_state(is);
}
