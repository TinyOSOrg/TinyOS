#include <kernel/syscall.h>

#include <kernel/console/console.h>
#include <kernel/explorer/explorer.h>
#include <kernel/filesys/dpt.h>
#include <kernel/filesys/syscall.h>
#include <kernel/driver/kbdriver.h>
#include <kernel/process/process.h>
#include <kernel/sysmsg/sysmsg_syscall.h>

#include <shared/syscall.h>

typedef void (*syscall_impl)();

syscall_impl syscall_func_table[SYSCALL_COUNT];

void init_syscall()
{
    // declared in kernel/process/process.h

    syscall_func_table[SYSCALL_GET_PROCESS_ID] =
        (syscall_impl)&syscall_get_cur_PID_impl;
    syscall_func_table[SYSCALL_YIELD_CPU] =
        (syscall_impl)&syscall_yield_CPU_impl;
    syscall_func_table[SYSCALL_EXIT_THREAD] =
        (syscall_impl)&syscall_thread_exit_impl;

    // declared in kernel/console/console.h

    syscall_func_table[SYSCALL_CONSOLE_OPERATION] =
        (syscall_impl)&syscall_console_impl;

    // declared in kernel/sysmsg/sysmsg_syscall.h

    syscall_func_table[SYSCALL_SYSMSG_OPERATION] =
        (syscall_impl)&syscall_sysmsg_impl;

    // declared in kernel/kbdriver.h
    
    syscall_func_table[SYSCALL_KEYBOARD_QUERY] =
        (syscall_impl)&syscall_keyboard_query_impl;
    
    // declared in kernel/filesys/syscall.h

    syscall_func_table[SYSCALL_FILESYS_OPEN] =
        (syscall_impl)&syscall_filesys_open_impl;
    syscall_func_table[SYSCALL_FILESYS_CLOSE] =
        (syscall_impl)&syscall_filesys_close_impl;
    syscall_func_table[SYSCALL_FILESYS_MKFILE] =
        (syscall_impl)&syscall_filesys_mkfile_impl;
    syscall_func_table[SYSCALL_FILESYS_RMFILE] =
        (syscall_impl)&syscall_filesys_rmfile_impl;
    syscall_func_table[SYSCALL_FILESYS_MKDIR] =
        (syscall_impl)&syscall_filesys_mkdir_impl;
    syscall_func_table[SYSCALL_FILESYS_RMDIR] =
        (syscall_impl)&syscall_filesys_rmdir_impl;
    syscall_func_table[SYSCALL_FILESYS_GET_SIZE] =
        (syscall_impl)&syscall_filesys_get_file_size_impl;
    syscall_func_table[SYSCALL_FILESYS_WRITE] =
        (syscall_impl)&syscall_filesys_write_impl;
    syscall_func_table[SYSCALL_FILESYS_READ] =
        (syscall_impl)&syscall_filesys_read_impl;
    syscall_func_table[SYSCALL_FILESYS_GET_CHILD_COUNT] =
        (syscall_impl)&syscall_filesys_get_file_count;
    syscall_func_table[SYSCALL_FILESYS_GET_CHILD_INFO] =
        (syscall_impl)&syscall_filesys_get_child_info;
    
    // declared in kernel/filesys/dpt.h
    
    syscall_func_table[SYSCALL_DP_GET_HANDLE] =
        (syscall_impl)&syscall_get_dp_handle_impl;

    // declared in kernel/explorer/explorer.h

    syscall_func_table[SYSCALL_EXPL_ALLOC_FOREGROUND] =
        (syscall_impl)&syscall_alloc_fg_impl;
    syscall_func_table[SYSCALL_EXPL_FREE_FOREGROUND] =
        (syscall_impl)&syscall_free_fg_impl;
    syscall_func_table[SYSCALL_EXPL_ALLOC_CON_BUF] =
        (syscall_impl)&syscall_alloc_con_buf_impl;
    syscall_func_table[SYSCALL_EXPL_PUT_CHAR] =
        (syscall_impl)&syscall_put_char_expl_impl;
    syscall_func_table[SYSCALL_EXPL_NEW_LINE] =
        (syscall_impl)&syscall_expl_new_line_impl;
    syscall_func_table[SYSCALL_PIPE_NULL_CHAR] =
        (syscall_impl)&syscall_expl_pipe_null_char;
}
