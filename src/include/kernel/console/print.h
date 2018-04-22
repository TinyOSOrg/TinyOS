#ifndef TINY_OS_PRINT_H
#define TINY_OS_PRINT_H

#include <shared/intdef.h>

/*
    设置光标位置
    assert(pos < 2000);
*/
void kset_cursor_pos(uint16_t pos);

/*
    设置光标位置
    assert(row < 25 && col < 80);
*/
void kset_cursor_row_col(uint8_t row, uint8_t col);

/*
    取得光标位置
    ret = 80 * row + col
*/
uint16_t kget_cursor_pos(void);

/*
    取得光标位置
    ret = (row << 8) | col
*/
uint16_t kget_cursor_row_col(void);

/* 屏幕滚动一行 */
void kroll_screen(void);

/*
    在光标位置处输出字符并自动调整光标
    对特殊字符，仅支持换行(\n)，退格(backspace)，水平制表(\t)
    光标超出屏幕范围时会自动滚屏
*/
void kput_char(char ch);

/* 等价于挨个put_char */
void kput_str(const char *str);

/*
    格式化输出
    %u: uint32_t
    %s: string
    %c: char
    %%: %
    \b: backspace
    \n: nextline
    \t: tab
    \\: backslash
    \": double quote
    其他字母、数字及符号原样输出

    不合法fmt将导致未定义行为
*/
void kprint_format(const char *fmt, ...);

/* ch就得是普通字符，控制字符一律UB */
void kset_char(uint16_t pos, char ch);

void kset_attrib(uint16_t pos, char attrib);

#endif /* TINY_OS_PRINT_H */
