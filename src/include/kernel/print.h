#ifndef TINY_OS_PRINT_H
#define TINY_OS_PRINT_H

#include <lib/integer.h>

// 设置光标位置
// assert(row < 25 && col < 80);
void _set_cursor_pos(uint8_t row, uint8_t col);

// 取得光标位置
// ret = 80 * row + col
uint16_t _get_cursor_pos(void);

// 取得光标位置
// ret = (row << 8) | col
uint16_t _get_cursor_row_col(void);

// 在光标位置处输出字符并自动调整光标
// 对特殊字符，仅支持换行(\n)，退格(backspace)
// 光标超出屏幕范围时会自动滚屏
void _put_char(uint8_t ch);

// 设置字符，不移动光标
// 特殊字符不做处理
void _set_char_at_cursor(uint8_t ch);

#endif //TINY_OS_PRINT_H
