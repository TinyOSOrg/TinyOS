#include <kernel/intr_entry.h>
#include <kernel/print.h>
#include <lib/string.h>

void print(const char *str)
{
    while(*str)
        put_char(*str++);
}

int main(void)
{
    set_cursor_pos(0, 0);
    init_IDT();
    asm volatile ("sti");
    
    while(1)
        ;
}
