#include <shared/syscall/common.h>
#include <shared/syscall/sysmsg.h>

#include <lib/sys.h>

bool has_sysmsg()
{
    return syscall_param1(SYSCALL_SYSMSG_OPERATION,
                          SYSMSG_SYSCALL_FUNCTION_IS_EMPTY)
           == false;
}

bool peek_sysmsg(uint32_t opr, struct sysmsg *msg)
{
    return syscall_param3(SYSCALL_SYSMSG_OPERATION,
                          SYSMSG_SYSCALL_FUNCTION_PEEK_MSG, opr, msg)
           != false;
}

void wait_for_sysmsg()
{
    syscall_param1(SYSCALL_SYSMSG_OPERATION,
                   SYSMSG_SYSCALL_FUNCTION_BLOCK_ONTO_SYSMSG);
}
