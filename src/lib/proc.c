#include <shared/syscall/common.h>

#include <lib/sys.h>

uint32_t get_pid()
{
    return syscall_param0(SYSCALL_GET_PROCESS_ID);
}

void yield_cpu()
{
    syscall_param0(SYSCALL_YIELD_CPU);
}
