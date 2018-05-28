#include <kernel/assert.h>
#include <kernel/console/con_buf.h>
#include <kernel/memory.h>
#include <kernel/process/spinlock.h>

#include <shared/freelist.h>

#include <lib/string.h>

STATIC_ASSERT(sizeof(struct con_buf) < 4096, invalid_size_of_con_buf);

struct con_buf *alloc_con_buf()
{
    struct con_buf *ret = (struct con_buf*)alloc_ker_page(false);
    memset(ret->data, 0x0, CON_BUF_BYTE_SIZE);
    ret->cursor = 0;
    init_semaphore(&ret->lock, 1);
    return ret;
}

void free_con_buf(struct con_buf *buf)
{
    free_ker_page(buf);
}
