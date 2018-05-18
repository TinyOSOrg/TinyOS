#include <shared/intdef.h>
#include <shared/ptrlist.h>

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

void init_ilist(ilist *L)
{
    L->last = L->next = L;
}

void push_back_ilist(ilist *L, struct ilist_node *node)
{
    node->last = L->last;
    node->next = L;
    L->last->next = node;
    L->last = node;
}

struct ilist_node *pop_front_ilist(ilist *L)
{
    if(is_ilist_empty(L))
        return NULL;
    struct ilist_node *rt = L->next;
    rt->next->last = L;
    L->next = rt->next;
    return rt;
}

bool is_ilist_empty(ilist *L)
{
    return L->next == L;
}

void erase_from_ilist(struct ilist_node *node)
{
    node->last->next = node->next;
    node->next->last = node->last;
    node->last = node->next = NULL;
}
