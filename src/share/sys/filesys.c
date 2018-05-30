#include <shared/syscall.h>
#include <shared/sys.h>

enum filesys_opr_result open_file(filesys_dp_handle dp,
                                  const char *path, bool writing,
                                  usr_file_handle *result)
{
    struct syscall_filesys_open_params args =
    {
        .writing = writing,
        .dp      = dp,
        .path    = path,
        .result  = result
    };
    return syscall_param1(SYSCALL_FILESYS_OPEN, &args);
}

enum filesys_opr_result close_file(usr_file_handle file)
{
    return syscall_param1(SYSCALL_FILESYS_CLOSE, file);
}

enum filesys_opr_result make_file(filesys_dp_handle dp,
                                  const char *path)
{
    return syscall_param2(SYSCALL_FILESYS_MKFILE, dp, path);
}

enum filesys_opr_result remove_file(filesys_dp_handle dp,
                                    const char *path)
{
    return syscall_param2(SYSCALL_FILESYS_RMFILE, dp, path);
}

enum filesys_opr_result make_directory(filesys_dp_handle dp,
                                       const char *path)
{
    return syscall_param2(SYSCALL_FILESYS_MKDIR, dp, path);
}

enum filesys_opr_result remove_directory(filesys_dp_handle dp,
                                         const char *path)
{
    return syscall_param2(SYSCALL_FILESYS_RMDIR, dp, path);
}

uint32_t get_file_size(usr_file_handle file)
{
    return syscall_param1(SYSCALL_FILESYS_GET_SIZE, file);
}

enum filesys_opr_result write_file(usr_file_handle file,
                                  uint32_t fpos, uint32_t byte_size,
                                  const void *data)
{
    struct syscall_filesys_write_params args =
    {
        .file      = file,
        .fpos      = fpos,
        .byte_size = byte_size,
        .data_src  = data
    };
    return syscall_param1(SYSCALL_FILESYS_WRITE, &args);
}

enum filesys_opr_result read_file(usr_file_handle file,
                                  uint32_t fpos, uint32_t byte_size,
                                  void *data)
{
    struct syscall_filesys_read_params args =
    {
        .file      = file,
        .fpos      = fpos,
        .byte_size = byte_size,
        .data_dst  = data
    };
    return syscall_param1(SYSCALL_FILESYS_READ, &args);
}

enum filesys_opr_result get_child_file_count(filesys_dp_handle dp,
                                             const char *path,
                                             uint32_t *_rt)
{
    enum filesys_opr_result ret; uint32_t rt;
    ret = syscall_param3(SYSCALL_FILESYS_GET_CHILD_COUNT, dp, path, &rt);
    if(_rt) *_rt = rt;
    return ret;
}

enum filesys_opr_result get_child_file_info(filesys_dp_handle dp,
                                            const char *path,
                                            uint32_t idx,
                                            struct syscall_filesys_file_info *rt)
{
    struct syscall_filesys_get_child_info_params args =
    {
        .dp   = dp,
        .path = path,
        .idx  = idx,
        .info = rt
    };
    return syscall_param1(SYSCALL_FILESYS_GET_CHILD_INFO, &args);
}
