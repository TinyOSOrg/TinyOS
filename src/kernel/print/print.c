#include <kernel/asm.h>
#include <kernel/print.h>

#include <lib/bool.h>
#include <lib/string.h>

void set_cursor_pos(uint16_t pos)
{
    _out_byte_to_port(0x03d4, 0x0e);
    _out_byte_to_port(0x03d5, pos >> 8);
    _out_byte_to_port(0x03d4, 0x0f);
    _out_byte_to_port(0x03d5, pos & 0xff);
}

void set_cursor_row_col(uint8_t row, uint8_t col)
{
    set_cursor_pos(80 * row + col);
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

static inline void set_word(uint16_t cursor, uint8_t fst, uint8_t snd)
{
    char* addr  = (char*)0xc00b8000 + (cursor << 1);
    *addr       = fst;
    *(addr + 1) = snd;
}

void put_char(char ch)
{
    // 取得光标位置
    uint16_t cursor_pos = get_cursor_pos();
    
    if(ch == '\n' || ch == '\r') // 换行
    {
        cursor_pos += (80 - cursor_pos % 80);
    }
    else if(ch == '\b') // 退格
    {
        if(cursor_pos != 0)
            set_word(--cursor_pos, (uint8_t)' ', 0x07);
    }
    else if(ch == '\t') // 水平制表四空格，不服憋着
    {
        cursor_pos += (4 - (cursor_pos + 1) % 4);
    }
    else // 普通字符
    {
        set_word(cursor_pos++, (uint8_t)ch, 0x07);
    }

    // 滚屏
    if(cursor_pos >= 2000)
    {
        memcpy((char*)0xc00b8000, (char*)0xc00b80a0, 3840);
        memset((char*)(0xc00b8000 + 3840), 0x0, 160);
        cursor_pos = 1920;
    }

    // 更新光标位置
    set_cursor_pos(cursor_pos);
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
            case 'c':
                put_char((char)*(uint32_t*)next_param);
                next_param += 4;
                break;
            }
            ++fmt;
        }
        else
            put_char(*fmt++);
    }
}
