#include <kernel/asm.h>
#include <kernel/interrupt.h>
#include <kernel/memory.h>
#include <kernel/console/print.h>
#include <kernel/console/console.h>
#include <kernel/kbdriver.h>
#include <kernel/process/semaphore.h>
#include <kernel/process/process.h>
#include <kernel/rlist_node_alloc.h>
#include <kernel/syscall.h>
#include <kernel/sysmsg/sysmsg_syscall.h>
#include <kernel/sysmsg/sysmsg.h>

#include <lib/conio.h>
#include <lib/keycode.h>
#include <lib/string.h>

#define syscall_param0(N) \
    ({ uint32_t r; \
       asm volatile ("int $0x80;" \
                     : "=a" (r) \
                     : "a" (N) \
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

struct semaphore sph;

void PL0_thread3(void)
{
    semaphore_wait(&sph);
    kprint_format("another process 3, pid = %u\n",
        syscall_param0(SYSCALL_GET_PROCESS_ID));
    semaphore_signal(&sph);
    while(1)
        ;
    exit_thread();
}

void PL0_thread0(void)
{
    for(int i = 0;i != 3; ++i)
    {
        semaphore_wait(&sph);
        kprint_format("another process 0, pid = %u\n",
            syscall_param0(SYSCALL_GET_PROCESS_ID));
        semaphore_signal(&sph);
    }
    semaphore_wait(&sph);
    kprint_format("PL0_thread0 exit!\n");
    semaphore_signal(&sph);
    exit_thread();
}

void PL0_thread1(void)
{
    for(int i = 0;i != 5; ++i)
    {
        semaphore_wait(&sph);
        kprint_format("another process 1, pid = %u\n",
            syscall_param0(SYSCALL_GET_PROCESS_ID));
        semaphore_signal(&sph);
    }
    create_process("another process3", PL0_thread3, true);
    exit_thread();
}

void PL0_thread2(void)
{
    semaphore_wait(&sph);
    kprint_format("another process 2, pid = %u\n",
        syscall_param0(SYSCALL_GET_PROCESS_ID));
    semaphore_signal(&sph);
    syscall_param3(SYSCALL_SYSMSG_OPERATION, SYSMSG_SYSCALL_FUNCTION_REGISTER_CHAR_MSG, 0, 0);
    while(1)
    {
        struct sysmsg msg;
        if(syscall_param3(SYSCALL_SYSMSG_OPERATION,
                          SYSMSG_SYSCALL_FUNCTION_PEEK_MSG,
                          SYSMSG_SYSCALL_PEEK_OPERATION_REMOVE,
                          &msg))
        {
            if(msg.type == SYSMSG_TYPE_CHAR)
            {
                struct kbchar_msg_struct *chmsg = (struct kbchar_msg_struct*)&msg;
                semaphore_wait(&sph);
                kput_char(chmsg->ch);
                semaphore_signal(&sph);
            }
        }
    }
    exit_thread();
}

void init_kernel(void)
{
    /* 中断向量表 */
    init_IDT();

    /* 物理内存管理 */
    init_phy_mem_man();

    /* 虚拟内存管理 */
    init_vir_mem_man();

    /* 内核链表分配器 */
    init_kernel_rlist_node_alloc();

    /* 内核线程管理 */
    init_thread_man();

    /* 进程管理 */
    init_process_man();

    /* 系统调用框架 */
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
}

int main(void)
{
    init_kernel();

    kset_cursor_pos(0);

    init_semaphore(&sph, 1);
    
    create_process("another process0", PL0_thread0, true);
    create_process("another process1", PL0_thread1, true);
    create_process("another process2", PL0_thread2, true);

    _enable_intr();

    {
        semaphore_wait(&sph);
        uint32_t pid = syscall_param0(SYSCALL_GET_PROCESS_ID);
        kprint_format("main process, pid = %u\n", pid);
        semaphore_signal(&sph);
    }

    while(1)
    {
        do_releasing_thds_procs();
        yield_CPU();
    }
}
