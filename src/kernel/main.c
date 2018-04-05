#include <kernel/asm.h>
#include <kernel/intr_entry.h>
#include <kernel/phy_mem_man.h>
#include <kernel/print.h>
#include <kernel/vir_mem_man.h>

#include <lib/string.h>

void pretend_to_be_a_scheduler(void)
{
    put_str("clock!\n");
}

void page_fault(void)
{
    put_str("page fault!");
}

int main(void)
{
    set_cursor_pos(0, 0);
    
    init_IDT();
    init_phy_mem_man();
    init_vir_mem_man();

    intr_function[INTR_NUMBER_CLOCK] = pretend_to_be_a_scheduler;

    while(1)
        ;
}
