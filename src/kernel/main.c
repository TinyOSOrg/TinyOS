#include <kernel/asm.h>
#include <kernel/intr_entry.h>
#include <kernel/memory.h>
#include <kernel/print.h>

#include <lib/string.h>

void pretend_to_be_a_scheduler(void)
{
    put_str("clock!\n");
}

int main(void)
{
    set_cursor_pos(0);

    init_IDT();
    init_phy_mem_man();
    init_vir_mem_man();

    set_intr_function(INTR_NUMBER_CLOCK, pretend_to_be_a_scheduler);

    *(char*)0x500000 = 'A';

    vir_addr_space *usr_addr = create_vir_addr_space();
    set_current_vir_addr_space(usr_addr);
    
    *(char*)0x500000 = 'B';
    print_format("char output: %c\n", *(char*)0x500000);

    set_current_vir_addr_space(get_ker_vir_addr_space());
    destroy_vir_addr_space(usr_addr);
    
    print_format("char output: %c\n", *(char*)0x500000);

    //_enable_intr();

    while(1)
        ;
}
