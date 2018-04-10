#include <lib/intdef.h>
#include <lib/ptrlist.h>

#include <kernel/print.h>

void init_rlist(rlist *L)
{
    L->last = L->next = (struct rlist_node*)L;
}

void push_back_rlist(rlist *L, void *ptr, rlist_node_allocator node_alloc)
{
    struct rlist_node *new_node = node_alloc();

    new_node->ptr = ptr;
    new_node->next = (struct rlist_node*)L;
    new_node->last = L->last;

    L->last->next = new_node;
    L->last = new_node;

    //print_format("%u\n", ptr);
}

void *pop_back_rlist(rlist *L, rlist_node_deallocator node_dealloc)
{
    if(is_rlist_empty(L))
        return NULL;

    void *rt = L->last->ptr;
    struct rlist_node *back_last = L->last->last;
    back_last->next = (struct rlist_node*)L;
    node_dealloc(L->last);
    L->last = back_last;

    return rt;
}

void push_front_rlist(rlist *L, void *ptr, rlist_node_allocator node_alloc)
{
    struct rlist_node *new_node = node_alloc();

    new_node->ptr = ptr;
    new_node->next = L->next;
    new_node->last = (struct rlist_node*)L;

    L->next->last = new_node;
    L->next = new_node;
}

void *pop_front_rlist(rlist *L, rlist_node_deallocator node_dealloc)
{
    if(is_rlist_empty(L))
        return NULL;
    
    void *rt = L->next->ptr;
    struct rlist_node *front_next = L->next->next;
    front_next->last = (struct rlist_node*)L;
    node_dealloc(L->next);
    L->next = front_next;

    //print_format("%u\n", rt);
    return rt;
}

void *back_rlist(rlist *L)
{
    return is_rlist_empty(L) ? NULL : L->last->ptr;
}

void *front_rlist(rlist *L)
{
    return is_rlist_empty(L) ? NULL : L->next->ptr;
}

bool is_rlist_empty(rlist *L)
{
    return L->last == (struct rlist_node*)L;
}
