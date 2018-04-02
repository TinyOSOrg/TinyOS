#include <kernel/intr_entry.h>
#include <kernel/mem_man.h>
#include <kernel/print.h>
#include <lib/string.h>

void print(const char *str)
{
    while(*str)
        put_char(*str++);
}

void pretend_to_be_a_scheduler(void)
{
    put_str("clock!\n");
}

#include <kernel/asm.h>

int main(void)
{
    init_IDT();
    init_mem_man();

    set_cursor_pos(0, 0);

    intr_function[INTR_NUMBER_CLOCK] = pretend_to_be_a_scheduler;

    char output_str[20];
    _uint32_to_str(get_mem_total_bytes() / 0x100000, output_str);
    _strcat(output_str, "MB");
    put_str(output_str);
    
    //asm volatile ("sti");

    while(1)
        ;
}
