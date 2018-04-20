#include <kernel/syscall.h>

#include <kernel/console/console.h>
#include <kernel/kbdriver.h>
#include <kernel/process/process.h>
#include <kernel/sysmsg/sysmsg_syscall.h>

typedef void (*syscall_impl)(void);

syscall_impl syscall_func_table[SYSCALL_COUNT];

void init_syscall(void)
{
    // declared in kernel/process/process.h
    syscall_func_table[SYSCALL_GET_PROCESS_ID] =
        (syscall_impl)&syscall_get_cur_PID_impl;
    // declared in kernel/console/console.h
    syscall_func_table[SYSCALL_CONSOLE_OPERATION] =
        (syscall_impl)&syscall_console_impl;
    // declared in kernel/sysmsg/sysmsg_syscall.h
    syscall_func_table[SYSCALL_SYSMSG_OPERATION] =
        (syscall_impl)&syscall_sysmsg_impl;
    // declared in kernel/kbdriver.h
    syscall_func_table[SYSCALL_KEYBOARD_QUERY] =
        (syscall_impl)&syscall_keyboard_query_impl;
}
