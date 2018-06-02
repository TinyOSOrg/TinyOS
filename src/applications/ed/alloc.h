#ifndef ED_ALLOC_H
#define ED_ALLOC_H

#include <shared/ptrlist.h>

#include <lib/mem.h>

static inline struct rlist_node *alloc_rlist_node()
{
    return (struct rlist_node *)malloc(sizeof(struct rlist_node));
}

static inline void free_rlist_node(struct rlist_node *node)
{
    free(node);
}

#endif /* ED_ALLOC_H */
