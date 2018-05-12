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
#include <kernel/filesys/afs/disk_cache.h>
#include <kernel/filesys/afs/file.h>

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

int main()
{
    init_kernel();

    set_cursor_row_col(0, 0);

    create_process("another process", PL0_thread, true);

    _enable_intr();

    printf("main process, pid = %u\n",
        syscall_param0(SYSCALL_GET_PROCESS_ID));
    
    struct dpt_unit *dp = get_dpt_unit(0);
    afs_phy_reformat_dp(dp->sector_begin,
        dp->sector_end - dp->sector_begin);

    struct afs_dp_head dph;
    afs_init_dp_head(dp->sector_begin, &dph);

    // 建立一个空文件
    uint32_t file_idx = afs_create_empty_file(
        &dph, AFS_FILE_TYPE_REGULAR, NULL);
    
    struct afs_file_desc *fp = afs_open_file_for_writing(
        &dph, file_idx, NULL);

    printf("shit!\n");
    
    uint32_t file_size = 1024 * 5;
    afs_expand_file(&dph, fp, file_size, NULL);

    printf("shit!\n");

    for(uint32_t i = 0, j = 0;i < file_size; i += 4, ++j)
        afs_write_binary(&dph, fp, 1024 + i, 4, &j, NULL);

    printf("shit!\n");

    for(uint32_t i = 12; i < 72; i += 4)
    {
        uint32_t j = 0;
        afs_read_binary(&dph, fp, 1024 + i, 4, &j, NULL);
        printf("j = %u ", j);
    }

    printf("shit!\n");

    afs_close_file_for_writing(&dph, fp);

    while(1)
    {
        do_releasing_thds_procs();
        yield_CPU();
    }
}
