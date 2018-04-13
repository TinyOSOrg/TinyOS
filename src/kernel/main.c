#include <kernel/asm.h>
#include <kernel/interrupt.h>
#include <kernel/memory.h>
#include <kernel/print.h>
#include <kernel/process/semaphore.h>
#include <kernel/process/process.h>
#include <kernel/rlist_node_alloc.h>
#include <kernel/syscall.h>

#include <lib/string.h>

#define syscall_param0(N) \
    ({ int r; \
       asm volatile ("int $0x80;" \
                     : "=a" (r) \
                     : "a" (N) \
                     : "memory"); \
       r; })

struct semaphore sph;

void PL0_thread(void)
{
    while(1)
    {
        /*semaphore_wait(&sph);
        print_format("another process, pid = %u\n",
            syscall_param0(SYSCALL_GET_PROCESS_ID));
        semaphore_signal(&sph);*/
    }
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

    /* 系统调用 */
    init_syscall();

    /* 时钟中断频率 */
    set_8253_freq(50);
}

int main(void)
{
    set_cursor_pos(0);

    init_kernel();

    init_semaphore(&sph, 1);
    
    create_process("another process", PL0_thread, false);

    _enable_intr();

    while(1)
    {
        semaphore_wait(&sph);
        uint32_t pid = syscall_param0(SYSCALL_GET_PROCESS_ID);
        print_format("main process, pid = %u\n", pid);
        semaphore_signal(&sph);
    }
}
