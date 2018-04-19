#include <kernel/kbdriver.h>
#include <kernel/process/process.h>
#include <kernel/sysmsg/sysmsg_syscall.h>
#include <kernel/sysmsg/sysmsg.h>

#include <lib/bool.h>
#include <lib/string.h>

static uint32_t (*functions[SYSMSG_SYSCALL_FUNCTION_COUNT])(uint32_t, uint32_t);

static uint32_t proc_sysmsg_syscall_is_empty(uint32_t a, uint32_t b)
{
    return is_sysmsg_queue_empty(&get_cur_TCB()->pcb->sys_msgs);
}

static uint32_t proc_sysmsg_syscall_peek_msg(uint32_t opr, uint32_t output)
{
    struct sysmsg *msg = (struct sysmsg*)output;
    if(!msg)
        return false;

    struct sysmsg_queue *queue = &get_cur_TCB()->pcb->sys_msgs;
    if(is_sysmsg_queue_empty(queue))
        return false;

    if(opr & SYSMSG_SYSCALL_PEEK_OPERATION_REMOVE)
        pop_sysmsg_queue_front(queue, msg);
    else
        memcpy((char*)msg, (char*)fetch_sysmsg_queue_front(queue), sizeof(struct sysmsg));
    
    return true;
}

static uint32_t proc_sysmsg_syscall_register_kbmsg(uint32_t a, uint32_t b)
{
    subscribe_kb(get_cur_TCB()->pcb);
    return 0;
}

void init_sysmsg_syscall(void)
{
    functions[SYSMSG_SYSCALL_FUNCTION_IS_EMPTY] =
        proc_sysmsg_syscall_is_empty;
    functions[SYSMSG_SYSCALL_FUNCTION_PEEK_MSG] =
        proc_sysmsg_syscall_peek_msg;
    functions[SYSMSG_SYSCALL_FUNCTION_REGISTER_KEYBOARD_MSG] =
        proc_sysmsg_syscall_register_kbmsg;
}

uint32_t syscall_sysmsg_impl(uint32_t func, uint32_t arg1, uint32_t arg2)
{
    if(func >= SYSMSG_SYSCALL_FUNCTION_COUNT)
        return 0;
    return functions[func](arg1, arg2);
}
