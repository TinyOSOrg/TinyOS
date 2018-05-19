#include <kernel/assert.h>
#include <kernel/filesys/dpt.h>
#include <kernel/filesys/filesys.h>

#include <kernel/filesys/afs/afs.h>
#include <kernel/filesys/afs/file_type.h>

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

#define SET_RT(V) do { if(rt) *rt = (V); } while(0)

file_handle open_reading(filesys_dp_handle dp, const char *path,
                         enum filesys_opr_result *rt)
{
    struct dpt_unit *u = get_dpt_unit(dp);
    switch(u->type)
    {
    case DISK_PT_AFS:
        {
            enum afs_file_operation_status trt;
            struct afs_file_desc *ret =
                afs_open_regular_file_for_reading_by_path(
                    (struct afs_dp_head*)get_dp_fs_handler(dp), path, &trt);
            SET_RT(trans_afs_file_result(trt));
            return trt == afs_file_opr_success ? (file_handle)ret : 0;
        }
    
    default:
        FATAL_ERROR("invalid file handle");
    }

    return filesys_opr_others;
}

file_handle open_writing(filesys_dp_handle dp, const char *path,
                         enum filesys_opr_result *rt)
{
    struct dpt_unit *u = get_dpt_unit(dp);
    switch(u->type)
    {
    case DISK_PT_AFS:
        {
            enum afs_file_operation_status trt;
            struct afs_file_desc *ret =
                afs_open_regular_file_for_writing_by_path(
                    (struct afs_dp_head*)get_dp_fs_handler(dp), path, &trt);
            SET_RT(trans_afs_file_result(trt));
            return trt == afs_file_opr_success ? (file_handle)ret : 0;
        }
    
    default:
        FATAL_ERROR("invalid file handle");
    }

    return filesys_opr_others;
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

enum filesys_opr_result make_regular(filesys_dp_handle dp,
                                     const char *path)
{
    struct dpt_unit *u = get_dpt_unit(dp);
    switch(u->type)
    {
    case DISK_PT_AFS:
        {
            enum afs_file_operation_status trt;
            afs_create_regular_file_by_path(
                (struct afs_dp_head*)get_dp_fs_handler(dp),
                path, &trt);
            return trans_afs_file_result(trt);
        }
    
    default:
        FATAL_ERROR("invalid file handle");
    }

    return filesys_opr_others;
}

enum filesys_opr_result remove_regular(filesys_dp_handle dp,
                                       const char *path)
{
    struct dpt_unit *u = get_dpt_unit(dp);
    switch(u->type)
    {
    case DISK_PT_AFS:
        {
            enum afs_file_operation_status trt;
            afs_remove_file_by_path(
                (struct afs_dp_head*)get_dp_fs_handler(dp),
                path, AFS_FILE_TYPE_REGULAR, &trt);
            return trans_afs_file_result(trt);
        }
    
    default:
        FATAL_ERROR("invalid file handle");
    }

    return filesys_opr_others;
}

enum filesys_opr_result make_directory(filesys_dp_handle dp,
                                       const char *path)
{
    struct dpt_unit *u = get_dpt_unit(dp);
    switch(u->type)
    {
    case DISK_PT_AFS:
        {
            enum afs_file_operation_status trt;
            afs_create_dir_file_by_path(
                (struct afs_dp_head*)get_dp_fs_handler(dp),
                path, &trt);
            return trans_afs_file_result(trt);
        }
    
    default:
        FATAL_ERROR("invalid file handle");
    }

    return filesys_opr_others;
}

enum filesys_opr_result remove_directory(filesys_dp_handle dp,
                                         const char *path)
{
    struct dpt_unit *u = get_dpt_unit(dp);
    switch(u->type)
    {
    case DISK_PT_AFS:
        {
            enum afs_file_operation_status trt;
            afs_remove_file_by_path(
                (struct afs_dp_head*)get_dp_fs_handler(dp),
                path, AFS_FILE_TYPE_DIRECTORY, &trt);
            return trans_afs_file_result(trt);
        }
    
    default:
        FATAL_ERROR("invalid file handle");
    }

    return filesys_opr_others;
}