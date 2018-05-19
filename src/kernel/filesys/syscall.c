#include <kernel/assert.h>
#include <kernel/filesys/dpt.h>
#include <kernel/filesys/filesys.h>
#include <kernel/filesys/syscall.h>
#include <kernel/process/thread.h>

#include <shared/ctype.h>
#include <shared/filesys/dpt.h>
#include <shared/string.h>

static filesys_dp_handle get_dp_handle_from_path(const char **_path, bool *error)
{
    ASSERT_S(_path && error);

    const char *path = *_path;

    if(!path)
    {
        *error = true;
        return 0;
    }

    // 取得分区名字长度

    uint32_t dp_name_len = 0;
    while(path[dp_name_len] && path[dp_name_len] != '/' &&
          dp_name_len <= (DP_NAME_BUF_SIZE - 1))
        ++dp_name_len;

    if(!dp_name_len || !path[dp_name_len] ||
       dp_name_len > DP_NAME_BUF_SIZE - 1)
    {
        *error = true;
        return 0;
    }

    // 若分区名字最后一个字符是':'，则按数字寻址
    if(path[dp_name_len - 1] == ':')
    {
        filesys_dp_handle ret = 0;
        uint32_t end = dp_name_len - 1;

        if(!end)
        {
            *error = true;
            return 0;
        }

        for(uint32_t i = 0; i < end; ++i)
        {
            if(!isdigit(path[i]))
            {
                *error = true;
                return 0;
            }
            ret = 10 * ret + (path[i] - '0');
        }

        *error = false;
        *_path += dp_name_len;
        return ret;
    }

    // 老老实实在分区表中查找
    for(size_t i = 0; i != DPT_UNIT_COUNT; ++i)
    {
        const char *dp_name = get_dpt_unit(i)->name;
        uint32_t j = 0;
        while(j < dp_name_len)
        {
            if(path[j] != dp_name[j])
                goto next_dp_unit;
        }

        if(dp_name[j] == '\0')
        {
            *error = false;
            *_path += dp_name_len;
            return i;
        }

next_dp_unit:

        // 避免 label next_dp_unit 编译不过
        (void)0;
    }

    *error = true;
    return 0;
}

#define SET_RT(V) do { if(rt) *rt = (V); } while(0)

file_handle syscall_filesys_open_impl(bool writing, const char *path,
                                      enum filesys_opr_result *rt)
{
    // 取得分区号
    bool dp_error;
    filesys_dp_handle dp = get_dp_handle_from_path(&path, &dp_error);
    if(dp_error)
    {
        SET_RT(filesys_opr_others);
        return 0;
    }

    thread_syscall_protector_entry();

    file_handle ret = writing ? open_regular_writing(dp, path, rt) :
                                open_regular_reading(dp, path, rt);

    thread_syscall_protector_exit();
    return ret;
}

enum filesys_opr_result syscall_filesys_close_impl(filesys_dp_handle dp,
                                                   file_handle file)
{
    thread_syscall_protector_entry();

    enum filesys_opr_result ret = close_file(dp, file);

    thread_syscall_protector_exit();
    return ret;
}

enum filesys_opr_result syscall_filesys_mkfile_impl(const char *path)
{
    // 取得分区号
    bool dp_error;
    filesys_dp_handle dp = get_dp_handle_from_path(&path, &dp_error);
    if(dp_error)
        return filesys_opr_others;

    thread_syscall_protector_entry();

    enum filesys_opr_result ret = make_regular(dp, path);

    thread_syscall_protector_exit();
    return ret;
}

enum filesys_opr_result syscall_filesys_rmfile_impl(const char *path)
{
    // 取得分区号
    bool dp_error;
    filesys_dp_handle dp = get_dp_handle_from_path(&path, &dp_error);
    if(dp_error)
        return filesys_opr_others;

    thread_syscall_protector_entry();

    enum filesys_opr_result ret = remove_regular(dp, path);

    thread_syscall_protector_exit();
    return ret;
}

enum filesys_opr_result syscall_filesys_mkdir_impl(const char *path)
{
    // 取得分区号
    bool dp_error;
    filesys_dp_handle dp = get_dp_handle_from_path(&path, &dp_error);
    if(dp_error)
        return filesys_opr_others;

    thread_syscall_protector_entry();

    enum filesys_opr_result ret = make_directory(dp, path);

    thread_syscall_protector_exit();
    return ret;
}

enum filesys_opr_result syscall_filesys_rmdir_impl(const char *path)
{
    // 取得分区号
    bool dp_error;
    filesys_dp_handle dp = get_dp_handle_from_path(&path, &dp_error);
    if(dp_error)
        return filesys_opr_others;

    thread_syscall_protector_entry();

    enum filesys_opr_result ret = remove_directory(dp, path);

    thread_syscall_protector_exit();
    return ret;
}

uint32_t syscall_filesys_get_file_size_impl(filesys_dp_handle dp,
                                            file_handle file)
{
    thread_syscall_protector_entry();
    uint32_t ret = get_regular_size(dp, file);
    thread_syscall_protector_exit();
    return ret;
}

enum filesys_opr_result syscall_filesys_write_impl(
        struct syscall_filesys_write_params *params)
{
    thread_syscall_protector_entry();
    enum filesys_opr_result ret = write_to_regular(
                    params->dp, params->file,
                    params->fpos, params->byte_size,
                    params->data_src);
    thread_syscall_protector_exit();
    return ret;
}

enum filesys_opr_result syscall_filesys_read_impl(
        struct syscall_filesys_read_params *params)
{
    thread_syscall_protector_entry();
    enum filesys_opr_result ret = read_from_regular(
                    params->dp, params->file,
                    params->fpos, params->byte_size,
                    params->data_dst);
    thread_syscall_protector_exit();
    return ret;
}
