#include <kernel/asm.h>
#include <kernel/interrupt.h>
#include <kernel/memory.h>
#include <kernel/print.h>
#include <kernel/process/thread.h>

#include <lib/string.h>

void pretend_to_be_a_scheduler(void)
{
    put_str("c");
}

void thread_test(void *arg)
{
    while(true)
        put_str((char*)arg);
}

int main(void)
{
    set_cursor_pos(0);

    init_IDT();
    init_phy_mem_man();
    init_vir_mem_man();
    init_thread_man();

    set_8253_freq(10);

    set_intr_function(INTR_NUMBER_CLOCK, pretend_to_be_a_scheduler);

    create_thread(thread_test, "test");

    //_enable_intr();

    while(1)
        ;
}
