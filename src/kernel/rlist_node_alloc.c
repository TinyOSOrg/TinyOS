#include <kernel/memory.h>
#include <kernel/rlist_node_alloc.h>

#include <lib/freelist.h>

/* 记录空闲rlist node的自由链表 */
static freelist_handle unused_resident_rlist_nodes;

void init_kernel_rlist_node_alloc(void)
{
    init_freelist(&unused_resident_rlist_nodes);
}

struct rlist_node *kernel_resident_rlist_node_alloc(void)
{
    if(is_freelist_empty(&unused_resident_rlist_nodes))
    {
        struct rlist_node *new_nodes = (struct rlist_node*)alloc_ker_page(true);
        size_t end = 4096 / sizeof(struct rlist_node);
        for(size_t i = 0;i != end; ++i)
            add_freelist(&unused_resident_rlist_nodes, &new_nodes[i]);
    }
    return fetch_freelist(&unused_resident_rlist_nodes);
}

void kernel_resident_rlist_node_dealloc(struct rlist_node *node)
{
    add_freelist(&unused_resident_rlist_nodes, node);
}
