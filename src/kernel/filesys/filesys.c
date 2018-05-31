#include <kernel/assert.h>
#include <kernel/filesys/dpt.h>
#include <kernel/filesys/filesys.h>

#include <kernel/filesys/afs/afs.h>
#include <kernel/filesys/afs/file.h>
#include <kernel/filesys/afs/file_type.h>

#include <shared/filesys.h>
#include <shared/utility.h>

void init_filesys()
{
    init_afs();
}

void destroy_filesys()
{
    destroy_afs();
}

/*
    IMPROVE：这里的文件操作函数基本是switch把不同类型文件系统的操作
        分派到不同具体函数，可能会很慢而且还不灵活
        有时间优化成跳转表的形式
*/

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
    ASSERT((uint32_t)rt < ARRAY_SIZE(trans));

    return trans[(uint32_t)rt];
}

#define SET_RT(V) do { if(rt) *rt = (V); } while(0)

file_handle kopen_regular_reading(filesys_dp_handle dp, const char *path,
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
        {
            SET_RT(filesys_opr_invalid_dp);
            return 0;
        }
    }

    return 0;
}

file_handle kopen_regular_writing(filesys_dp_handle dp, const char *path,
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
        {
            SET_RT(filesys_opr_invalid_dp);
            return 0;
        }
    }

    return filesys_opr_others;
}

uint32_t kget_child_file_count(filesys_dp_handle dp, const char *path,
                               enum filesys_opr_result *rt)
{
    struct dpt_unit *u = get_dpt_unit(dp);
    switch(u->type)
    {
    case DISK_PT_AFS:
        {
            enum afs_file_operation_status trt;
            struct afs_dp_head *dp_head = (struct afs_dp_head*)get_dp_fs_handler(dp);
            struct afs_file_desc *dir = afs_open_dir_file_for_reading_by_path(
                                            dp_head, path, &trt);
            if(!dir)
            {
                SET_RT(trans_afs_file_result(trt));
                return 0;
            }
            
            uint32_t ret;
            if(!afs_read_binary(dp_head, dir, 0, 4, &ret, &trt))
                ret = 0;

            afs_close_file(dp_head, dir);
            SET_RT(filesys_opr_success);
            return ret;
        }
    
    default:
        {
            SET_RT(filesys_opr_invalid_dp);
            return 0;
        }
    }

    SET_RT(filesys_opr_others);
    return 0;
}

enum filesys_opr_result kget_child_file_info(filesys_dp_handle dp, const char *path, uint32_t idx,
                                             struct syscall_filesys_file_info *info)
{
    struct dpt_unit *u = get_dpt_unit(dp);
    switch(u->type)
    {
    case DISK_PT_AFS:
        {
            enum afs_file_operation_status trt;
            struct afs_dp_head *dp_head = (struct afs_dp_head*)get_dp_fs_handler(dp);
            struct afs_file_desc *dir = afs_open_dir_file_for_reading_by_path(
                                            dp_head, path, &trt);
            if(!dir)
                return trans_afs_file_result(trt);

            trt = afs_get_dir_unit(dp_head, dir, idx, info);

            afs_close_file(dp_head, dir);
            return trans_afs_file_result(trt);
        }
    
    default:
        {
            return filesys_opr_invalid_dp;
        }
    }

    return filesys_opr_others;
}

enum filesys_opr_result kclose_file(filesys_dp_handle dp, file_handle file_handle)
{
    struct dpt_unit *u = get_dpt_unit(dp);
    switch(u->type)
    {
    case DISK_PT_AFS:
        afs_close_file((struct afs_dp_head*)get_dp_fs_handler(dp),
                       (struct afs_file_desc*)file_handle);
        return trans_afs_file_result(afs_file_opr_success);

    default:
        return filesys_opr_invalid_dp;
    }

    return filesys_opr_others;
}

enum filesys_opr_result kmake_regular(filesys_dp_handle dp, const char *path)
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
        return filesys_opr_invalid_dp;
    }

    return filesys_opr_others;
}

enum filesys_opr_result kremove_regular(filesys_dp_handle dp, const char *path)
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
        return filesys_opr_invalid_dp;
    }

    return filesys_opr_others;
}

enum filesys_opr_result kmake_directory(filesys_dp_handle dp, const char *path)
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
        return filesys_opr_invalid_dp;
    }

    return filesys_opr_others;
}

enum filesys_opr_result kremove_directory(filesys_dp_handle dp, const char *path)
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
        return filesys_opr_invalid_dp;
    }

    return filesys_opr_others;
}

/* 取得一个文件的字节数 */
uint32_t kget_regular_size(filesys_dp_handle dp, file_handle file)
{
    struct dpt_unit *u = get_dpt_unit(dp);
    switch(u->type)
    {
    case DISK_PT_AFS:
        {
            return afs_get_file_byte_size((struct afs_file_desc*)file);
        }
    
    default:
        return 0;
    }

    return 0;
}

enum filesys_opr_result kwrite_to_regular(
                            filesys_dp_handle dp, file_handle file,
                            uint32_t fpos, uint32_t size,
                            const void *data)
{
    uint32_t old_size = kget_regular_size(dp, file);
    if(!size || fpos > old_size)
        return filesys_opr_out_of_range;

    struct dpt_unit *u = get_dpt_unit(dp);
    switch(u->type)
    {
    case DISK_PT_AFS:
        {
            struct afs_dp_head *head =
                (struct afs_dp_head*)get_dp_fs_handler(dp);
            struct afs_file_desc *fp =
                (struct afs_file_desc*)file;

            enum afs_file_operation_status trt;

            uint32_t pot_new_size = fpos + size;
            if(pot_new_size > old_size)
            {
                if(!afs_expand_file(head, fp, pot_new_size, &trt))
                    return trans_afs_file_result(trt);
            }

            if(!afs_write_binary(head, fp, fpos, size, data, &trt))
                return trans_afs_file_result(trt);
            
            return filesys_opr_success;
        }

    default:
        return filesys_opr_invalid_dp;
    }

    return filesys_opr_others;
}

enum filesys_opr_result kread_from_regular(
                            filesys_dp_handle dp, file_handle file,
                            uint32_t fpos, uint32_t size,
                            void *data)
{
    struct dpt_unit *u = get_dpt_unit(dp);
    switch(u->type)
    {
    case DISK_PT_AFS:
        {
            enum afs_file_operation_status trt;
            afs_read_binary((struct afs_dp_head*)get_dp_fs_handler(dp),
                            (struct afs_file_desc*)file,
                            fpos, size, data, &trt);
            return trans_afs_file_result(trt);
        }
    
    default:
        return filesys_opr_invalid_dp;
    }

    return filesys_opr_others;
}
