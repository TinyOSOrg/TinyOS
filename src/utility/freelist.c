#include <shared/freelist.h>
#include <shared/intdef.h>

#define FL ((struct freelist_node*)handle)

struct freelist_node
{
    struct freelist_node *next;
};

void init_freelist(freelist_handle *handle)
{
    FL->next = NULL;
}

void add_freelist(freelist_handle *handle, void *mem_zone)
{
    struct freelist_node *node = (struct freelist_node*)mem_zone;
    node->next = FL->next;
    FL->next = node;
}

bool is_freelist_empty(freelist_handle *handle)
{
    return FL->next == NULL;
}

void *fetch_freelist(freelist_handle *handle)
{
    if(is_freelist_empty(handle))
        return NULL;
    void *rt = FL->next;
    FL->next = FL->next->next;
    return rt;
}
