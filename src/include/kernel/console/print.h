#ifndef TINY_OS_PRINT_H
#define TINY_OS_PRINT_H

#include <kernel/console/con_buf.h>

#include <shared/intdef.h>

/*
    设置光标位置
    assert(pos < 2000);
*/
void kset_cursor_pos(struct con_buf *buf, uint16_t pos);

/*
    设置光标位置
    assert(row < 25 && col < 80);
*/
void kset_cursor_row_col(struct con_buf *buf, uint8_t row, uint8_t col);

/*
    取得光标位置
    ret = 80 * row + col
*/
uint16_t kget_cursor_pos(struct con_buf *buf);

/*
    取得光标位置
    ret = (row << 8) | col
*/
uint16_t kget_cursor_row_col(struct con_buf *buf);

/* 屏幕滚动一行 */
void kroll_screen(struct con_buf *buf);

/* 指定的行范围 */
void kroll_screen_row_between(struct con_buf *buf,
                              uint32_t beg, uint32_t end);

/*
    在光标位置处输出字符并自动调整光标
    对特殊字符，仅支持换行(\n)，退格(backspace)，水平制表(\t)
    光标超出屏幕范围时会自动滚屏
*/
void kput_char(struct con_buf *buf, char ch);

char kget_char(struct con_buf *buf, uint16_t pos);

void kput_str(struct con_buf *buf, const char *str);

/* ch就得是普通字符，控制字符一律UB */
void kset_char(struct con_buf *buf, uint16_t pos, char ch);

void kset_attrib(struct con_buf *buf, uint16_t pos, char attrib);

#endif /* TINY_OS_PRINT_H */
