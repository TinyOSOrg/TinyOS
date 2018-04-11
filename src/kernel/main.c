#include <kernel/asm.h>
#include <kernel/interrupt.h>
#include <kernel/memory.h>
#include <kernel/print.h>
#include <kernel/process/semaphore.h>
#include <kernel/process/thread.h>
#include <kernel/rlist_node_alloc.h>

#include <lib/string.h>

struct semaphore scr_sph;

void thread_test(void *arg)
{
    while(true)
    {
        semaphore_wait(&scr_sph);
        put_str("thrd ");
        semaphore_signal(&scr_sph);
    }
}

int main(void)
{
    set_cursor_pos(0);

    init_IDT();
    init_phy_mem_man();
    init_vir_mem_man();
    init_kernel_rlist_node_alloc();
    init_thread_man();

    set_8253_freq(10);

    init_semaphore(&scr_sph, 1);
    create_thread(thread_test, NULL);

    _enable_intr();

    while(1)
    {
        semaphore_wait(&scr_sph);
        put_str("main ");
        semaphore_signal(&scr_sph);
    }
}
