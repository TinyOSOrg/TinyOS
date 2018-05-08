#include <kernel/asm.h>
#include <kernel/assert.h>
#include <kernel/interrupt.h>
#include <kernel/memory.h>
#include <kernel/console/print.h>
#include <kernel/console/console.h>
#include <kernel/diskdriver.h>
#include <kernel/kbdriver.h>
#include <kernel/process/semaphore.h>
#include <kernel/process/process.h>
#include <kernel/rlist_node_alloc.h>
#include <kernel/syscall.h>
#include <kernel/sysmsg/sysmsg_syscall.h>
#include <kernel/sysmsg/sysmsg.h>

#include <kernel/filesys/afs/afs.h>
#include <kernel/filesys/dpt.h>
#include <kernel/filesys/afs/sector_cache.h>

#include <lib/conio.h>

#include <shared/keycode.h>
#include <shared/rbtree.h>
#include <shared/string.h>
#include <shared/syscall/common.h>
#include <shared/syscall/sysmsg.h>
#include <shared/sysmsg/kbmsg.h>

#define syscall_param0(N) \
    ({ uint32_t r; \
       asm volatile ("int $0x80;" \
                     : "=a" (r) \
                     : "a" (N) \
                     : "memory"); \
       r; })

#define syscall_param1(N, arg1) \
    ({ uint32_t r; \
       asm volatile ("int $0x80" \
                     : "=a" (r) \
                     : "a" (N), "b" (arg1) \
                     : "memory"); \
       r; })

#define syscall_param2(N, arg1, arg2) \
    ({ uint32_t r; \
       asm volatile ("int $0x80" \
                     : "=a" (r) \
                     : "a" (N), "b" (arg1), "c" (arg2) \
                     : "memory"); \
       r; })

#define syscall_param3(N, arg1, arg2, arg3) \
    ({ uint32_t r; \
       asm volatile ("int $0x80" \
                     : "=a" (r) \
                     : "a" (N), "b" (arg1), "c" (arg2), "d" (arg3) \
                     : "memory"); \
       r; })

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

    /* 硬盘驱动 */
    init_disk_driver();

    /* 磁盘分区 */
    init_dpt();

    /* afs文件系统 */
    init_afs();
}

int main(void)
{
    init_kernel();

    set_cursor_row_col(0, 0);

    create_process("another process", PL0_thread, true);

    _enable_intr();

    printf("main process, pid = %u\n", syscall_param0(SYSCALL_GET_PROCESS_ID));

    char buf[512];
    for(int i = 0;i != 40; ++i)
    {
        memset(buf, i, 512);
        afs_write_to_sector(320 + i, 0, 512, buf);
    }

    afs_release_all_sector_cache();

    for(int i = 0;i != 40; ++i)
    {
        printf("r%u: ", i);
        afs_read_from_sector(320 + i, 256, 20, buf);
        printf("%u \n", (uint32_t)buf[10]);
    }

    while(1)
    {
        do_releasing_thds_procs();
        yield_CPU();
    }
}
