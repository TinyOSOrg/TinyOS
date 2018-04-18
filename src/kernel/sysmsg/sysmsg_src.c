#include <kernel/assert.h>
#include <kernel/memory.h>
#include <kernel/sysmsg/sysmsg_src.h>

#include <lib/freelist.h>

/* 用来分配sysmsg_rcv_src_list_node的自由链表 */
static freelist_handle sysmsg_rcv_src_list_node_freelist;

struct sysmsg_rcv_src_list_node *alloc_sysmsg_rcv_src_list_node(void)
{
    if(is_freelist_empty(&sysmsg_rcv_src_list_node_freelist))
    {
        struct sysmsg_rcv_src_list_node *new_nodes =
            (struct sysmsg_rcv_src_list_node *)alloc_ker_page(false);
        for(size_t i = 0;i < 4096 / sizeof(struct sysmsg_rcv_src_list_node); ++i)
            add_freelist(&sysmsg_rcv_src_list_node_freelist,
                         &new_nodes[i]);
    }
    return fetch_freelist(&sysmsg_rcv_src_list_node_freelist);
}

void destroy_sysmsg_receiver_list(struct sysmsg_receiver_list *L)
{
    ASSERT_S(L);
    
    while(!is_ilist_empty(&L->processes))
    {
        struct ilist_node *ori_node = pop_front_ilist(&L->processes);
        struct sysmsg_rcv_src_list_node *dst_node =
            GET_STRUCT_FROM_MEMBER(struct sysmsg_rcv_src_list_node,
                                   rcv_node, ori_node);
        erase_from_ilist(&dst_node->src_node);
        add_freelist(&sysmsg_rcv_src_list_node_freelist, dst_node);
    }
}

void destroy_sysmsg_source_list(struct sysmsg_source_list *L)
{
    ASSERT_S(L);

    while(!is_ilist_empty(&L->sources))
    {
        struct ilist_node *ori_node = pop_front_ilist(&L->sources);
        struct sysmsg_rcv_src_list_node *dst_node =
            GET_STRUCT_FROM_MEMBER(struct sysmsg_rcv_src_list_node,
                                   src_node, ori_node);
        erase_from_ilist(&dst_node->rcv_node);
        add_freelist(&sysmsg_rcv_src_list_node_freelist, dst_node);
    }
}

void register_sysmsg_source(struct PCB *pcb,
                            struct sysmsg_receiver_list *rcv,
                            struct sysmsg_source_list   *src)
{
    ASSERT_S(rcv && src);

    struct sysmsg_rcv_src_list_node *node = alloc_sysmsg_rcv_src_list_node();
    push_back_ilist(&rcv->processes, &node->rcv_node);
    push_back_ilist(&src->sources, &node->src_node);
    node->pcb = pcb;
}
