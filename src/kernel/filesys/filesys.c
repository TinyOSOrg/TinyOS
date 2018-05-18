#include <kernel/assert.h>
#include <kernel/filesys/dpt.h>
#include <kernel/filesys/filesys.h>

#include <kernel/filesys/afs/afs.h>

#include <shared/utility.h>

static enum filesys_opr_result trans_afs_file_result(
    enum afs_file_operation_status rt)
{
    const static enum filesys_opr_result trans[] =
    {
        [afs_file_opr_success]          = filesys_opr_success,
        [afs_file_opr_writing_lock]     = filesys_opr_locked,
        [afs_file_opr_reading_lock]     = filesys_opr_locked,
        [afs_file_opr_not_opening]      = filesys_opr_others,
        [afs_file_opr_limit_exceeded]   = filesys_opr_out_of_range,
        [afs_file_opr_no_empty_entry]   = filesys_opr_no_disk_space,
        [afs_file_opr_no_empty_block]   = filesys_opr_no_disk_space,
        [afs_file_opr_read_only]        = filesys_opr_read_only,
        [afs_file_opr_invalid_new_size] = filesys_opr_others,
        [afs_file_opr_not_found]        = filesys_opr_not_found,
        [afs_file_opr_rm_nonempty]      = filesys_opr_rm_nonempty,
        [afs_file_opr_file_existed]     = filesys_opr_existed,
        [afs_file_opr_rm_locked]        = filesys_opr_locked,
        [afs_file_opr_rm_wrong_type]    = filesys_opr_not_found
    };
    ASSERT_S((uint32_t)rt < ARRAY_SIZE(trans));

    return trans[(uint32_t)rt];
}

enum filesys_opr_result close_file(filesys_dp_handle dp,
                                  file_handle file_handle)
{
    struct dpt_unit *u = get_dpt_unit(dp);
    switch(u->type)
    {
    case DISK_PT_AFS:
        afs_close_file((struct afs_dp_head*)get_dp_fs_handler(dp),
                       (struct afs_file_desc*)file_handle);
        return trans_afs_file_result(afs_file_opr_success);

    default:
        FATAL_ERROR("invalid file handle");
    }

    return filesys_opr_others;
}
