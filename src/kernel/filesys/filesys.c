#include <kernel/assert.h>
#include <kernel/filesys/dpt.h>
#include <kernel/filesys/filesys.h>

#include <kernel/filesys/afs/afs.h>

void close_file(size_t dp_idx, uint32_t file_handle)
{
    struct dpt_unit *u = get_dpt_unit(dp_idx);
    switch(u->type)
    {
    case DISK_PT_AFS:
        afs_close_file((struct afs_dp_head*)get_dp_fs_handler(dp_idx),
                       (struct afs_file_desc*)file_handle);
        break;
    default:
        FATAL_ERROR("invalid file handle");
    }
}
