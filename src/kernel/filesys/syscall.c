#include <kernel/assert.h>
#include <kernel/filesys/dpt.h>
#include <kernel/filesys/filesys.h>
#include <kernel/filesys/syscall.h>
#include <kernel/interrupt.h>
#include <kernel/process/process.h>

#include <shared/ctype.h>
#include <shared/filesys.h>
#include <shared/string.h>

static bool get_file_from_usr_handle(usr_file_handle handle,
                                    filesys_dp_handle *dp,
                                    file_handle *file)
{
    ASSERT_S(dp && file);

    struct PCB *pcb = get_cur_TCB()->pcb;
    spinlock_lock(&pcb->file_table_lock);

    if(!is_atrc_unit_valid(&pcb->file_table,
                           ATRC_ELEM_SIZE(struct pcb_file_record),
                           handle))
    {
        spinlock_unlock(&pcb->file_table_lock);
        return false;
    }

    struct pcb_file_record *rcd = get_atrc_unit(
                                    &pcb->file_table,
                                    ATRC_ELEM_SIZE(struct pcb_file_record),
                                    handle);
    *dp   = rcd->dp;
    *file = rcd->file;

    spinlock_unlock(&pcb->file_table_lock);

    return true;
}

enum filesys_opr_result syscall_filesys_open_impl(
        struct syscall_filesys_open_params *params)
{
    // 检查参数合法性

    if(params->dp >= DPT_UNIT_COUNT || !params->path)
        return filesys_opr_others;

    thread_syscall_protector_entry();

    struct PCB *pcb = get_cur_TCB()->pcb;

    // 申请一个进程文件记录

    spinlock_lock(&pcb->file_table_lock);
    usr_file_handle usr_handle = alloc_atrc_unit(
        &get_cur_TCB()->pcb->file_table,
        ATRC_ELEM_SIZE(struct pcb_file_record));
    spinlock_unlock(&pcb->file_table_lock);

    if(usr_handle == ATRC_ELEM_HANDLE_NULL)
    {
        thread_syscall_protector_exit();
        return filesys_opr_file_table_full;
    }

    // 尝试打开文件

    enum filesys_opr_result ret;

    intr_state is = fetch_and_enable_intr();
    file_handle handle = params->writing ?
        kopen_regular_writing(
            params->dp, params->path, &ret) :
        kopen_regular_reading(
            params->dp, params->path, &ret);
    set_intr_state(is);

    // 若打开失败，回滚操作

    if(ret != filesys_opr_success)
    {
        spinlock_lock(&pcb->file_table_lock);
        free_atrc_unit(&pcb->file_table,
                       ATRC_ELEM_SIZE(struct pcb_file_record),
                       usr_handle);
        spinlock_unlock(&pcb->file_table_lock);

        thread_syscall_protector_exit();
        return ret;
    }

    // 录入进程文件记录内容

    spinlock_lock(&pcb->file_table_lock);
    struct pcb_file_record *rcd = get_atrc_unit(
                                    &pcb->file_table,
                                    ATRC_ELEM_SIZE(struct pcb_file_record),
                                    usr_handle);
    rcd->dp   = params->dp;
    rcd->file = handle;
    spinlock_unlock(&pcb->file_table_lock);

    if(params->result)
        *params->result = usr_handle;

    thread_syscall_protector_exit();

    return ret;
}

enum filesys_opr_result syscall_filesys_close_impl(usr_file_handle file)
{
    thread_syscall_protector_entry();

    struct PCB *pcb = get_cur_TCB()->pcb;

    filesys_dp_handle dp;
    file_handle handle;
    if(!get_file_from_usr_handle(file, &dp, &handle))
    {
        thread_syscall_protector_exit();
        return filesys_opr_invalid_args;
    }

    intr_state is = fetch_and_enable_intr();
    enum filesys_opr_result ret = kclose_file(dp, handle);
    set_intr_state(is);

    free_atrc_unit(&pcb->file_table,
        ATRC_ELEM_SIZE(struct pcb_file_record), file);

    spinlock_unlock(&pcb->file_table_lock);

    thread_syscall_protector_exit();
    return ret;
}

enum filesys_opr_result syscall_filesys_mkfile_impl(filesys_dp_handle dp,
                                                    const char *path)
{
    thread_syscall_protector_entry();

    intr_state is = fetch_and_enable_intr();
    enum filesys_opr_result ret = kmake_regular(dp, path);
    set_intr_state(is);

    thread_syscall_protector_exit();
    return ret;
}

enum filesys_opr_result syscall_filesys_rmfile_impl(filesys_dp_handle dp,
													const char *path)
{
    thread_syscall_protector_entry();

    intr_state is = fetch_and_enable_intr();
    enum filesys_opr_result ret = kremove_regular(dp, path);
    set_intr_state(is);

    thread_syscall_protector_exit();
    return ret;
}

enum filesys_opr_result syscall_filesys_mkdir_impl(filesys_dp_handle dp,
												   const char *path)
{
    thread_syscall_protector_entry();

    intr_state is = fetch_and_enable_intr();
    enum filesys_opr_result ret = kmake_directory(dp, path);
    set_intr_state(is);

    thread_syscall_protector_exit();
    return ret;
}

enum filesys_opr_result syscall_filesys_rmdir_impl(filesys_dp_handle dp,
												   const char *path)
{
    thread_syscall_protector_entry();

    intr_state is = fetch_and_enable_intr();
    enum filesys_opr_result ret = kremove_directory(dp, path);
    set_intr_state(is);

    thread_syscall_protector_exit();
    return ret;
}

uint32_t syscall_filesys_get_file_size_impl(usr_file_handle handle)
{
    thread_syscall_protector_entry();

    filesys_dp_handle dp;
    file_handle file;
    if(!get_file_from_usr_handle(handle, &dp, &file))
    {
        thread_syscall_protector_exit();
        return 0;
    }

    intr_state is = fetch_and_enable_intr();
    uint32_t ret = kget_regular_size(dp, file);
    set_intr_state(is);

    thread_syscall_protector_exit();
    return ret;
}

enum filesys_opr_result syscall_filesys_write_impl(
        struct syscall_filesys_write_params *params)
{
    thread_syscall_protector_entry();

    filesys_dp_handle dp;
    file_handle file;
    if(!get_file_from_usr_handle(params->file, &dp, &file))
    {
        thread_syscall_protector_exit();
        return filesys_opr_invalid_args;
    }

    intr_state is = fetch_and_enable_intr();
    enum filesys_opr_result ret = kwrite_to_regular(
                    dp, file,
                    params->fpos, params->byte_size,
                    params->data_src);
    set_intr_state(is);

    thread_syscall_protector_exit();
    return ret;
}

enum filesys_opr_result syscall_filesys_read_impl(
        struct syscall_filesys_read_params *params)
{
    thread_syscall_protector_entry();

    filesys_dp_handle dp;
    file_handle file;
    if(!get_file_from_usr_handle(params->file, &dp, &file))
    {
        thread_syscall_protector_exit();
        return filesys_opr_invalid_args;
    }

    intr_state is = fetch_and_enable_intr();
    enum filesys_opr_result ret = kread_from_regular(
                    dp, file,
                    params->fpos, params->byte_size,
                    params->data_dst);
    set_intr_state(is);

    thread_syscall_protector_exit();
    return ret;
}

enum filesys_opr_result syscall_filesys_get_file_count(
            filesys_dp_handle dp, const char *path,
            uint32_t *rt)
{
    thread_syscall_protector_entry();
    enum filesys_opr_result ret;

    intr_state is = fetch_and_enable_intr();
    uint32_t _rt = kget_child_file_count(dp, path, &ret);
    set_intr_state(is);

    thread_syscall_protector_exit();
    if(rt) *rt = _rt;
    return ret;
}

enum filesys_opr_result syscall_filesys_get_child_info(
            struct syscall_filesys_get_child_info_params *params)
{
    thread_syscall_protector_entry();
    enum filesys_opr_result ret;

    intr_state is = fetch_and_enable_intr();
    ret = kget_child_file_info(
        params->dp, params->path, params->idx, params->info);
    set_intr_state(is);

    thread_syscall_protector_exit();
    return ret;
}
