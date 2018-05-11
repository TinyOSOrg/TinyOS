#include <kernel/asm.h>
#include <kernel/assert.h>
#include <kernel/interrupt.h>
#include <kernel/memory.h>
#include <kernel/console/print.h>
#include <kernel/console/console.h>
#include <kernel/diskdriver.h>
#include <kernel/filesys/dpt.h>
#include <kernel/kbdriver.h>
#include <kernel/process/semaphore.h>
#include <kernel/process/process.h>
#include <kernel/rlist_node_alloc.h>
#include <kernel/syscall.h>
#include <kernel/sysmsg/sysmsg_syscall.h>
#include <kernel/sysmsg/sysmsg.h>

#include <lib/conio.h>

#include <shared/keycode.h>
#include <shared/rbtree.h>
#include <shared/string.h>
#include <shared/syscall/common.h>
#include <shared/syscall/sysmsg.h>
#include <shared/sysmsg/kbmsg.h>


#include <kernel/drive_disk/drive_disk.h>
#include <kernel/memory/phy_mem_man.h>
/*
void PL0_thread(void)
{
    printf("hahaha process, pid = %u\n",
        syscall_param0(SYSCALL_GET_PROCESS_ID));
    syscall_param1(SYSCALL_SYSMSG_OPERATION, SYSMSG_SYSCALL_FUNCTION_REGISTER_CHAR_MSG);
    while(1)
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
                struct kbchar_msg_struct *chmsg = (struct kbchar_msg_struct*)&msg;
                put_char(chmsg->ch);
            }
        }
    }
    exit_thread();
}
*/
void init_kernel(void)
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
    set_8253_freq(100);

    /* 键盘驱动 */
    init_kb_driver();

    /*磁盘驱动*/
    init_drive_disk();

    /* 磁盘分区 */
    init_dpt(); 
}

int main(void)
{
    init_kernel();  
/*
    create_process("another process0", PL0_thread0, true);
    create_process("another process1", PL0_thread1, true);
    create_process("another process2", PL0_thread2, true);
*/
/*
    while(1)
    {
        do_releasing_thds_procs();
        yield_CPU();
    }
*/
    kprintf_format("begin\n");
    uint16_t *memptr1 = alloc_ker_page(true);
    uint16_t *memptr2 = alloc_ker_page(true);
    uint16_t *ptr = memptr1;
    for(uint32_t i = 0; i < 256; ++i)
    {
        *ptr++ = 28;
    }
    write_disk(ptr, 1000);
    read_disk(1000, memptr2);
    for(uint32_t i = 0; i < 256; ++i)
    {
        kprintf_format("%d ", *memptr2++);
    }
    kprintf_format("\nend\n");
    while(1)
    {
        ;
    }
}
