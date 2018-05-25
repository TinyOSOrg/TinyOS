#include <kernel/asm.h>
#include <kernel/explorer/explorer.h>
#include <kernel/kernel.h>
#include <kernel/process/process.h>

#include <shared/syscall/common.h>
#include <shared/syscall/sysmsg.h>
#include <shared/sysmsg/kbmsg.h>

#include <lib/conio.h>
#include <lib/keyboard.h>
#include <lib/proc.h>

void PL0_thread()
{
    get_cur_PCB()->disp_buf = alloc_con_buf();
    get_cur_PCB()->pis      = pis_foreground;
    
    printf("Keyboard process, pid = %u\n", get_pid());
    register_char_msg();

    while(true)
    {
        syscall_param1(SYSCALL_SYSMSG_OPERATION,
                       SYSMSG_SYSCALL_FUNCTION_BLOCK_ONTO_SYSMSG);
        struct sysmsg msg;
        while(syscall_param3(SYSCALL_SYSMSG_OPERATION,
                             SYSMSG_SYSCALL_FUNCTION_PEEK_MSG,
                             SYSMSG_SYSCALL_PEEK_OPERATION_REMOVE,
                             &msg))
        {
            if(msg.type == SYSMSG_TYPE_CHAR)
            {
                struct kbchar_msg_struct *chmsg =
                    (struct kbchar_msg_struct*)&msg;
                put_char(chmsg->ch);
            }
        }
    }
    exit_thread();
}

int main()
{
    init_kernel();

    create_process("explorer", explorer, true);

    create_process("another process", PL0_thread, true);

    _enable_intr();

    while(1)
    {
        do_releasing_thds_procs();
        yield_CPU();
    }
}
