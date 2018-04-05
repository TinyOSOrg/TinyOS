#include <kernel/asm.h>
#include <kernel/intr_entry.h>
#include <kernel/phy_mem_man.h>
#include <kernel/print.h>

#include <lib/string.h>

void pretend_to_be_a_scheduler(void)
{
    put_str("clock!\n");
}

int main(void)
{
    init_IDT();
    init_phy_mem_man();

    set_cursor_pos(0, 0);

    intr_function[INTR_NUMBER_CLOCK] = pretend_to_be_a_scheduler;
    
    uint32_t pages[80];
    
    print_format("free pages = %u\n", get_free_phy_page_count());

    for(int i = 0;i != 80; ++i)
        pages[i] = alloc_phy_page(true);

    print_format("free pages = %u\n", get_free_phy_page_count());

    for(int i = 0;i != 80; ++i)
        free_phy_page(pages[i]);

    print_format("free pages = %u\n", get_free_phy_page_count());

    while(1)
        ;
}
