#include <kernel/asm.h>
#include <kernel/assert.h>
#include <kernel/print.h>

#include <lib/bool.h>
#include <lib/string.h>

void _fatal_error_impl(const char *prefix, const char *filename, const char *function, int line, const char *msg)
{
    _disable_intr();
    
    put_str(prefix);
    put_str(filename);
    put_str(", ");
    put_str(function);
    put_str(", ");

    char int_str_buf[40];
    uint32_to_str(line, int_str_buf);
    put_str(int_str_buf);
    
    put_str(": ");
    put_str(msg);

    while(true)
        ;
}
