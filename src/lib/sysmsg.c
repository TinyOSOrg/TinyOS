#include <shared/syscall.h>
#include <shared/sysmsg.h>

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

void clr_sysmsgs()
{
    struct sysmsg msg;
    while(peek_sysmsg(SYSMSG_SYSCALL_PEEK_OPERATION_REMOVE, &msg))
        ;
}
