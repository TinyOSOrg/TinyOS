#include <kernel/asm.h>
#include <kernel/assert.h>

#include <kernel/console/print.h>

#include <lib/bool.h>
#include <lib/string.h>

void _fatal_error_impl(const char *prefix, const char *filename, const char *function, int line, const char *msg)
{
    _disable_intr();

    kset_cursor_pos(0);
    
    kput_char('\n');
    kput_str(prefix);
    kput_str(filename);
    kput_str(", ");
    kput_str(function);
    kput_str(", ");

    char int_str_buf[40];
    uint32_to_str(line, int_str_buf);
    kput_str(int_str_buf);
    
    kput_str(": ");
    kput_str(msg);

    while(true)
        ;
}
