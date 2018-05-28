#include <kernel/asm.h>
#include <kernel/assert.h>
#include <kernel/console/print.h>

#include <shared/bool.h>

#include <lib/string.h>

void kset_cursor_pos(struct con_buf *buf, uint16_t pos)
{
    ASSERT_S(pos < CON_BUF_CHAR_COUNT);
    buf->cursor = pos;
}

void kset_cursor_row_col(struct con_buf *buf, uint8_t row, uint8_t col)
{
    kset_cursor_pos(buf, 80 * row + col);
}

uint16_t kget_cursor_pos(struct con_buf *buf)
{
    return buf->cursor;
}

uint16_t kget_cursor_row_col(struct con_buf *buf)
{
    uint8_t row, col;
    uint16_t pos = kget_cursor_pos(buf);
    row = pos / 80;
    col = pos % 80;
    return (row << 8) | col;
}

void kroll_screen_row_between(struct con_buf *buf,
                              uint32_t beg, uint32_t end)
{
    memcpy(buf->data + (beg * 2 * CON_BUF_ROW_SIZE),
           buf->data + ((beg + 1) * 2 * CON_BUF_ROW_SIZE),
           (end - beg - 1) * 2 * CON_BUF_ROW_SIZE);
    char *d = buf->data + (end - 1) * 2 * CON_BUF_ROW_SIZE;
    for(uint32_t i = 0; i < CON_BUF_ROW_SIZE; ++i)
        d[i << 1] = ' ';
}

void kroll_screen(struct con_buf *buf)
{
    kroll_screen_row_between(buf, 0, CON_BUF_COL_SIZE);
}

static inline void kset_word(struct con_buf *buf, uint16_t cursor, uint8_t fst, uint8_t snd)
{
    char* addr  = buf->data + (cursor << 1);
    *addr       = fst;
    *(addr + 1) = snd;
}

void kput_char(struct con_buf *buf, char ch)
{
    // 取得光标位置
    uint16_t cursor_pos = kget_cursor_pos(buf);
    
    if(ch == '\n' || ch == '\r') // 换行
    {
        cursor_pos += (80 - cursor_pos % 80);
    }
    else if(ch == '\b') // 退格
    {
        if(cursor_pos != 0)
            kset_word(buf, --cursor_pos, (uint8_t)' ', 0x07);
    }
    else if(ch == '\t') // 水平制表四空格，不服憋着
    {
        cursor_pos += (4 - (cursor_pos + 1) % 4);
    }
    else // 普通字符
    {
        kset_word(buf, cursor_pos++, (uint8_t)ch, 0x07);
    }

    // 滚屏
    if(cursor_pos >= 2000)
    {
        kroll_screen(buf);
        cursor_pos = 1920;
    }

    // 更新光标位置
    kset_cursor_pos(buf, cursor_pos);
}

char kget_char(struct con_buf *buf, uint16_t pos)
{
    if(2 * pos < CON_BUF_BYTE_SIZE)
        return buf->data[pos * 2];
    return'\0';
}

void kput_str(struct con_buf *buf, const char *str)
{
    while(*str)
        kput_char(buf, *str++);
}

void kset_char(struct con_buf *buf, uint16_t pos, char ch)
{
    *(buf->data + (pos << 1)) = ch;
}

void kset_attrib(struct con_buf *buf, uint16_t pos, char attrib)
{
    *(buf->data + (pos << 1) + 1) = attrib;
}
