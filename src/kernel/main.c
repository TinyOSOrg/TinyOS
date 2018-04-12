#include <kernel/asm.h>
#include <kernel/interrupt.h>
#include <kernel/memory.h>
#include <kernel/print.h>
#include <kernel/process/semaphore.h>
#include <kernel/process/process.h>
#include <kernel/rlist_node_alloc.h>

#include <lib/string.h>

struct semaphore sph;

volatile int x;

void thread_test(void)
{
    while(1)
        ++x;
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

    /* 时钟中断频率 */
    set_8253_freq(50);
}

int main(void)
{
    set_cursor_pos(0);

    init_kernel();

    init_semaphore(&sph, 1);
    x = 0;
    create_process("test thread", thread_test);

    _enable_intr();

    while(1)
    {
        semaphore_wait(&sph);
        print_format("%u\n", x);
        semaphore_signal(&sph);
    }
}
