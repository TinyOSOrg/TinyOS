#include <kernel/asm.h>
#include <kernel/console/console.h>
#include <kernel/diskdriver.h>
#include <kernel/filesys/dpt.h>
#include <kernel/filesys/filesys.h>
#include <kernel/interrupt.h>
#include <kernel/kbdriver.h>
#include <kernel/memory.h>
#include <kernel/process/process.h>
#include <kernel/readelf.h>
#include <kernel/rlist_node_alloc.h>
#include <kernel/syscall.h>
#include <kernel/sysmsg/sysmsg_syscall.h>
#include <kernel/sysmsg/sysmsg.h>

#include <lib/conio.h>
#include <lib/filesys.h>

#include <shared/keycode.h>
#include <shared/rbtree.h>
#include <shared/string.h>
#include <shared/syscall/common.h>
#include <shared/syscall/sysmsg.h>
#include <shared/sysmsg/kbmsg.h>

#include <kernel/filesys/import/import.h>

void PL0_thread()
{
    printf("hahaha process, pid = %u\n",
        syscall_param0(SYSCALL_GET_PROCESS_ID));
    syscall_param1(SYSCALL_SYSMSG_OPERATION,
                   SYSMSG_SYSCALL_FUNCTION_REGISTER_CHAR_MSG);

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

void init_kernel()
{
    /* 中断系统 */
    init_IDT();

    /* 物理内存 */
    init_phy_mem_man();

    /* 虚拟内存 */
    init_vir_mem_man();

    /* 内核链表分配 */
    init_kernel_rlist_node_alloc();

    /* 内核线程 */
    init_thread_man();

    /* 进程管理 */
    init_process_man();

    /* 系统调用 */
    init_syscall();

    /* 控制台 */
    init_console();

    /* 内核消息传递 */
    init_sysmsg();

    /* 内核消息系统调用 */
    init_sysmsg_syscall();

    /* 时钟中断频率 */
    set_8253_freq(20);

    /* 键盘驱动 */
    init_kb_driver();

    /* 硬盘驱动 */
    init_disk_driver();

    /* 磁盘分区 */
    init_dpt();

    /* 文件系统 */
    init_filesys();
}

int main()
{
    init_kernel();

    set_cursor_row_col(0, 0);

    create_process("another process", PL0_thread, true);

    _enable_intr();

    printf("main process, pid = %u\n",
        syscall_param0(SYSCALL_GET_PROCESS_ID));
    
    reformat_dp(0, DISK_PT_AFS);

    ipt_import_from_dp(get_dpt_unit(DPT_UNIT_COUNT - 1)->sector_begin);

    uint8_t elf_data[9000];

    usr_file_handle fp;
    
    open_file(0, "/minecraft.txt", false, &fp);

    printf("file size = %u\n", get_file_size(fp));

    read_file(fp, 0, get_file_size(fp), elf_data);

    int (*entry_addr)() = (int(*)())load_elf(elf_data);
    entry_addr();

    close_file(fp);
    
    while(1)
    {
        do_releasing_thds_procs();
        yield_CPU();
    }
}
