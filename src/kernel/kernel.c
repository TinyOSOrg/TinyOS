#include <kernel/kernel.h>

#include <kernel/console/console.h>
#include <kernel/driver/diskdriver.h>
#include <kernel/filesys/dpt.h>
#include <kernel/filesys/filesys.h>
#include <kernel/interrupt.h>
#include <kernel/driver/kbdriver.h>
#include <kernel/memory.h>
#include <kernel/process/process.h>
#include <kernel/rlist_node_alloc.h>
#include <kernel/syscall.h>
#include <kernel/sysmsg/sysmsg_syscall.h>

void init_kernel()
{
    /* 时钟中断频率 */
    set_8253_freq(100);

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

    /* 键盘驱动 */
    init_kb_driver();

    /* 硬盘驱动 */
    init_disk_driver();

    /* 文件系统 */
    init_filesys();

    /* 磁盘分区 */
    init_dpt();

    _enable_intr();
}

void destroy_kernel()
{
    kill_all_processes();
    destroy_dpt();
    destroy_filesys();
}
