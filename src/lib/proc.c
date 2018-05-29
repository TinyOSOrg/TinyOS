#include <shared/syscall.h>

#include <lib/sys.h>

uint32_t get_pid()
{
    return syscall_param0(SYSCALL_GET_PROCESS_ID);
}

void yield_cpu()
{
    syscall_param0(SYSCALL_YIELD_CPU);
}

void exit_thread()
{
    syscall_param0(SYSCALL_EXIT_THREAD);
}
