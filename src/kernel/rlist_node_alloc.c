#include <kernel/memory.h>
#include <kernel/interrupt.h>
#include <kernel/rlist_node_alloc.h>

#include <shared/freelist.h>

/* 记录空闲rlist node的自由链表 */
static freelist_handle unused_resident_rlist_nodes;

void init_kernel_rlist_node_alloc()
{
    init_freelist(&unused_resident_rlist_nodes);
}

/*
    这个函数可能被多个进程并行调用，而那些进程很可能是不关中断的
    所以这里的alloc和free都要关中断操作
*/
struct rlist_node *kernel_resident_rlist_node_alloc()
{
    intr_state is = fetch_and_disable_intr();

    if(is_freelist_empty(&unused_resident_rlist_nodes))
    {
        struct rlist_node *new_nodes = (struct rlist_node*)alloc_ker_page(true);
        size_t end = 4096 / sizeof(struct rlist_node);
        for(size_t i = 0;i != end; ++i)
            add_freelist(&unused_resident_rlist_nodes, &new_nodes[i]);
    }

    struct rlist_node *ret = (struct rlist_node*)fetch_freelist(&unused_resident_rlist_nodes);
    set_intr_state(is);
    return ret;    
}

void kernel_resident_rlist_node_dealloc(struct rlist_node *node)
{
    intr_state is = fetch_and_disable_intr();
    add_freelist(&unused_resident_rlist_nodes, node);
    set_intr_state(is);
}
