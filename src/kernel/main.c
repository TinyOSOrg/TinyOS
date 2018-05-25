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
#include <lib/sysmsg.h>

void PL0_thread()
{
    get_cur_PCB()->disp_buf = alloc_con_buf();
    get_cur_PCB()->pis      = pis_foreground;
    
    printf("Keyboard process, pid = %u\n", get_pid());
    register_char_msg();

    while(true)
    {
        wait_for_sysmsg();
        struct sysmsg msg;
        while(peek_sysmsg(SYSMSG_SYSCALL_PEEK_OPERATION_REMOVE,
                          &msg))
        {
            if(msg.type == SYSMSG_TYPE_CHAR)
                put_char(((struct kbchar_msg_struct*)&msg)->ch);
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
