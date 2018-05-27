#include <kernel/syscall.h>

#include <kernel/console/console.h>
#include <kernel/filesys/syscall.h>
#include <kernel/driver/kbdriver.h>
#include <kernel/process/process.h>
#include <kernel/sysmsg/sysmsg_syscall.h>

#include <shared/syscall/common.h>

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
}
