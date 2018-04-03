#include <kernel/asm.h>
#include <kernel/print.h>

#include <lib/string.h>

void set_cursor_pos(uint8_t row, uint8_t col)
{
    uint16_t pos = 80 * row + col;
    _out_byte_to_port(0x03d4, 0x0e);
    _out_byte_to_port(0x03d5, pos >> 8);
    _out_byte_to_port(0x03d4, 0x0f);
    _out_byte_to_port(0x03d5, pos & 0xff);
}

uint16_t get_cursor_pos(void)
{
    uint8_t high8;
    _out_byte_to_port(0x03d4, 0x0e);
    high8 = _in_byte_from_port(0x03d5);
    _out_byte_to_port(0x03d4, 0x0f);
    return (high8 << 8) | _in_byte_from_port(0x03d5);
}

uint16_t get_cursor_row_col(void)
{
    uint8_t row, col;
    uint16_t pos = get_cursor_pos();
    row = pos / 80;
    col = pos % 80;
    return (row << 8) | col;
}

void put_str(const char *str)
{
    while(*str)
        put_char(*str++);
}

void print_format(const char *fmt, ...)
{
    const char *next_param = (const char *)&fmt + 4;
    char int_buf[32];
    while(*fmt)
    {
        if(*fmt == '%')
        {
            switch(*++fmt)
            {
            case 'u':
                uint32_to_str(*(uint32_t*)next_param, int_buf);
                put_str(int_buf);
                next_param += 4;
                break;
            case 's':
                put_str(*(char**)next_param);
                next_param += 4;
                break;
            case '%':
                put_char('%');
                break;
            }
            ++fmt;
        }
        else
            put_char(*fmt++);
    }
}
