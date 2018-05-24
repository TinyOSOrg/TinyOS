#include <kernel/console/con_buf.h>
#include <kernel/filesys/dpt.h>
#include <kernel/filesys/import/import.h>
#include <kernel/interrupt.h>
#include <kernel/kernel.h>
#include <kernel/memory.h>
#include <kernel/process/process.h>
#include <kernel/exec_elf/exec_elf.h>

#include <lib/conio.h>
#include <lib/filesys.h>
#include <lib/keyboard.h>
#include <lib/proc.h>

static void init_explorer()
{
    intr_state is = fetch_and_disable_intr();

    // explorer进程永远处于“前台”，以遍接收键盘消息
    // 话虽如此，并不能随便输出东西到系统控制台上

    struct PCB *pcb = get_cur_PCB();
    pcb->pis      = pis_foreground;
    pcb->disp_buf = alloc_con_buf();

    // 把自己从PCB列表中摘除
    erase_from_ilist(&pcb->processes_node);

    set_intr_state(is);
}

void explorer_entry()
{
    init_explorer();

    printf("Explorer process, pid = %u\n", get_pid());

    reformat_dp(0, DISK_PT_AFS);

    ipt_import_from_dp(get_dpt_unit(DPT_UNIT_COUNT - 1)->sector_begin);

    exec_elf("test elf", 0, "/minecraft.txt", false, 0, NULL);

    while(1)
    {
        if(is_key_pressed(VK_ESCAPE))
        {
            destroy_kernel();
            while(1)
                ;
        }

        yield_CPU();
    }
}
