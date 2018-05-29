#include <shared/screen.h>
#include <shared/syscall.h>

#include <lib/string.h>
#include <lib/sys.h>

#define CONSOLE_SYSCALL(N, arg) \
    do { uint32_t dummy_ret; \
         __asm__ __volatile__ ("int $0x80" \
                             : "=a" (dummy_ret) \
                             : "a" (SYSCALL_CONSOLE_OPERATION), \
                               "b" (N), \
                               "c" (arg) \
                             : "memory"); } while(0)

#define CONSOLE_SYSCALL_RET(N, arg, ret) \
    do { \
        __asm__ __volatile__ ("int $0x80" \
                            : "=a" (ret) \
                            : "a" (SYSCALL_CONSOLE_OPERATION), \
                              "b" (N), \
                              "c" (arg) \
                            : "memory"); } while(0)

void set_char_row_col(uint8_t row, uint8_t col, char ch)
{
    uint32_t arg = ((row * 80 + col) << 16) | (uint8_t)ch;
    CONSOLE_SYSCALL(CONSOLE_SYSCALL_FUNCTION_SET_CHAR, arg);
}

void set_char_at(uint16_t pos, char ch)
{
    set_char_row_col(pos / 80, pos % 80, ch);
}

void set_char_attrib_row_col(uint8_t row, uint8_t col, uint8_t attrib)
{
    uint32_t arg = ((row * 80 + col) << 16) | (uint8_t)attrib;
    CONSOLE_SYSCALL(CONSOLE_SYSCALL_FUNCTION_SET_ATTRIB, arg);
}

void get_cursor_row_col(uint8_t *row, uint8_t *col)
{
    uint32_t rt;
    CONSOLE_SYSCALL_RET(CONSOLE_SYSCALL_FUNCTION_GET_CURSOR, 0, rt);
    if(row)
        *row = (uint8_t)(rt / 80);
    if(col)
        *col = (uint8_t)(rt % 80);
}

void set_cursor_row_col(uint8_t row, uint8_t col)
{
    uint32_t arg = 80 * row + col;
    CONSOLE_SYSCALL(CONSOLE_SYSCALL_FUNCTION_SET_CURSOR, arg);
}

char get_char_row_col(uint8_t row, uint8_t col)
{
    uint32_t arg = 80 * row + col;
    uint32_t ret;
    CONSOLE_SYSCALL_RET(CONSOLE_SYSCALL_FUNCTION_GET_CHAR, arg, ret);
    return (char)ret;
}

void put_char(char ch)
{
    uint32_t arg = ch;
    CONSOLE_SYSCALL(CONSOLE_SYSCALL_FUNCTION_PUT_CHAR, arg);
}

void put_str(const char *str)
{
    while(*str)
        put_char(*str++);
}

void printf(const char *fmt, ...)
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
            case 'c':
                put_char((char)*(uint32_t*)next_param);
                next_param += 4;
                break;
            case '\0':
                return;
            }
            ++fmt;
        }
        else
            put_char(*fmt++);
    }
}

void roll_scr(uint32_t beg_row, uint32_t end_row)
{
    uint32_t arg = (((beg_row & 0xff) << 8) | (end_row & 0xff)) & 0xffff;
    CONSOLE_SYSCALL(CONSOLE_SYSCALL_FUNCTION_ROLL_SCREEN_BETWEEN, arg);
}
