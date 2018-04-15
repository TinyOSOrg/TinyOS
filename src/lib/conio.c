#include <kernel/console/console.h>
#include <kernel/syscall.h>

#include <lib/conio.h>

#define CONSOLE_SYSCALL(N, arg) \
    asm volatile ("int $0x80" \
                  : \
                  : "a" (SYSCALL_CONSOLE_OPERATION), \
                    "b" (N), \
                    "c" (arg) \
                  : "memory");

#define CONSOLE_SYSCALL_RET(N, arg, ret) \
    asm volatile ("int $0x80" \
                  : "=a" (ret) \
                  : "a" (SYSCALL_CONSOLE_OPERATION), \
                    "b" (N), \
                    "c" (arg) \
                  : "memory");

void set_char_row_col(uint8_t row, uint8_t col, char ch)
{
    uint32_t arg = ((row * 80 + col) << 16) | (uint8_t)ch;
    CONSOLE_SYSCALL(CONSOLE_SYSCALL_FUNCTION_SET_CHAR, arg);
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
