#include <kernel/filesys/afs/afs.h>
#include <kernel/filesys/afs/blk_mem_buf.h>
#include <kernel/filesys/afs/dp_phy.h>

void init_afs(void)
{
    init_afs_buffer_allocator();
}
