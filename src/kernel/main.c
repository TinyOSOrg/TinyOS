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

int main(void)
{
    set_cursor_pos(0, 0);
    init_IDT();

    intr_function[INTR_NUMBER_CLOCK] = (void (*)(uint8_t))pretend_to_be_a_scheduler;
    
    asm volatile ("sti");

    while(1)
        ;
}
