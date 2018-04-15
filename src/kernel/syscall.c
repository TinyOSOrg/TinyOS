#include <kernel/syscall.h>

#include <kernel/console/console.h>
#include <kernel/process/process.h>

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
}
