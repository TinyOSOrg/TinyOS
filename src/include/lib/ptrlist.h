#ifndef TINY_OS_PTRLIST_H
#define TINY_OS_PTRLIST_H

#include <lib/bool.h>

/* 循环链表节点 */
struct rlist_node
{
    struct rlist_node *last;
    struct rlist_node *next;
    void *ptr;
};

struct rlist_handle
{
    struct rlist_node *last;
    struct rlist_node *next;
};

/* 循环链表句柄 */
typedef struct rlist_handle rlist;

void init_rlist(rlist *L);

/* 循环链表节点分配器 */
typedef struct rlist_node *(*rlist_node_allocator)(void);
/* 循环链表节点释放器 */
typedef void (*rlist_node_deallocator)(struct rlist_node*);

void push_back_rlist(rlist *L, void *ptr, rlist_node_allocator node_alloc);
void *pop_back_rlist(rlist *L, rlist_node_deallocator node_dealloc);

void push_front_rlist(rlist *L, void *ptr, rlist_node_allocator node_alloc);
void *pop_front_rlist(rlist *L, rlist_node_deallocator node_dealloc);

void *back_rlist(rlist *L);
void *front_rlist(rlist *L);

bool is_rlist_empty(rlist *L);

#endif /* TINY_OS_PTRLIST_H */
