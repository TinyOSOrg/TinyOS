#include <shared/syscall/common.h>

#include <lib/proc.h>

uint32_t get_pid()
{
    return syscall_param0(SYSCALL_GET_PROCESS_ID);
}

void yield_cpu()
{
    return syscall_param0(SYSCALL_YIELD_CPU);
}
