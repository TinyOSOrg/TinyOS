#include <kernel/filesys/afs/disk_cache.h>
#include <kernel/interrupt.h>
#include <kernel/memory.h>

#include <shared/freelist.h>
#include <shared/ptrlist.h>
#include <shared/rbtree.h>

struct LRUnode
{
    struct ilist_node list_node;
    struct rb_node    tree_node;
    void *buffer;
};

/* 扇区缓存记录，linked map，节点类型为LRUnode */
static ilist sec_list;
static struct rb_tree sec_tree;

/* block缓存记录，linked map，节点类型为LRUnode */
static ilist blk_list;
static struct rb_tree blk_tree;

/* 空闲LRUnode的自由链表 */
static freelist_handle empty_LRUnodes;

/*
    分配一个空的LRUnode
    这个函数以及下面的free函数都可能在多个不同进程中被调用
    而操作时间又很短，因此关中断来保护freelist的完整性

    // TODO：换成自旋锁
*/
static struct LRUnode *alloc_LRUnode(void)
{
    intr_state is = fetch_and_disable_intr();

    if(is_freelist_empty(&empty_LRUnodes))
    {
        struct LRUnode *nodes = (struct LRUnode*)alloc_ker_page(true);
        size_t end = 4096 / sizeof(struct LRUnode);
        for(size_t i = 0;i < end; ++i)
            add_freelist(&empty_LRUnodes, &nodes[i]);
    }

    struct LRUnode *ret = (struct LRUnode*)fetch_freelist(&empty_LRUnodes);
    set_intr_state(is);
    return ret;
}

static void free_LRUnode(struct LRUnode *node)
{
    intr_state is = fetch_and_disable_intr();
    add_freelist(&empty_LRUnodes, node);
    set_intr_state(is);
}

void init_afs_disk_cache(void)
{
    init_ilist(&sec_list);
    rb_init(&sec_tree);

    init_ilist(&blk_list);
    rb_init(&blk_tree);

    init_freelist(&empty_LRUnodes);
}
