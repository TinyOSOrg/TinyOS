#ifndef TINY_OS_PRINT_H
#define TINY_OS_PRINT_H

#include <lib/intdef.h>

/*
    设置光标位置
    assert(row < 25 && col < 80);
*/
void set_cursor_pos(uint8_t row, uint8_t col);

/*
    取得光标位置
    ret = 80 * row + col
*/
uint16_t get_cursor_pos(void);

/*
    取得光标位置
    ret = (row << 8) | col
*/
uint16_t get_cursor_row_col(void);

/*
    在光标位置处输出字符并自动调整光标
    对特殊字符，仅支持换行(\n)，退格(backspace)
    光标超出屏幕范围时会自动滚屏
*/
void put_char(char ch);

void put_str(const char *str);

/*
    格式化输出
    %u: uint32_t
    %s: string
    %c: char
    %%: %
    \b: backspace
    \n: crlf
    \\: backslash
    \": double quote
    其他字母、数字及符号原样输出

    不合法fmt将导致未定义行为
*/
void print_format(const char *fmt, ...);

#endif /* TINY_OS_PRINT_H */
