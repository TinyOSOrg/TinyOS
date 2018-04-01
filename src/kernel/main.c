#include <kernel/intr_entry.h>
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
    set_cursor_pos(0, 0);
    init_IDT();

    intr_function[INTR_NUMBER_CLOCK] = pretend_to_be_a_scheduler;

    char output_str[20];
    _uint32_to_str(_find_highest_nonzero_bit(0b00010101000000), output_str);
    put_str(output_str);
    
    //asm volatile ("sti");

    while(1)
        ;
}
