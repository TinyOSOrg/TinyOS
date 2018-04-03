#include <kernel/asm.h>
#include <kernel/intr_entry.h>
#include <kernel/mem_man.h>
#include <kernel/print.h>

#include <lib/string.h>

void pretend_to_be_a_scheduler(void)
{
    put_str("clock!\n");
}

int main(void)
{
    init_IDT();
    init_mem_man();

    set_cursor_pos(0, 0);

    intr_function[INTR_NUMBER_CLOCK] = pretend_to_be_a_scheduler;
    
    char intbuf[50];
    uint32_t pages[80];
    
    uint32_to_str(get_free_phy_page_count(), intbuf);
    put_str(intbuf); put_char('\n');

    for(int i = 0;i != 80; ++i)
        pages[i] = alloc_phy_page(true);

    uint32_to_str(get_free_phy_page_count(), intbuf);
    put_str(intbuf); put_char('\n');

    for(int i = 0;i != 80; ++i)
        free_phy_page(pages[i]);

    uint32_to_str(get_free_phy_page_count(), intbuf);
    put_str(intbuf); put_char('\n');

    while(1)
        ;
}
