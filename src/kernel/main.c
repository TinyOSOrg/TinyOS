#include <kernel/intr_entry.h>
#include <kernel/print.h>
#include <lib/string.h>

void print(const char *str)
{
    while(*str)
        _put_char(*str++);
}

int main(void)
{
    _set_cursor_pos(0, 0);
    init_IDT();
    asm volatile ("sti");

    char c = 'A';
    while(1)
        ;
}
