#include <kernel/asm.h>
#include <kernel/console/print.h>

#include <lib/bool.h>
#include <lib/string.h>

void kset_cursor_pos(uint16_t pos)
{
    _out_byte_to_port(0x03d4, 0x0e);
    _out_byte_to_port(0x03d5, pos >> 8);
    _out_byte_to_port(0x03d4, 0x0f);
    _out_byte_to_port(0x03d5, pos & 0xff);
}

void kset_cursor_row_col(uint8_t row, uint8_t col)
{
    kset_cursor_pos(80 * row + col);
}

uint16_t kget_cursor_pos(void)
{
    uint8_t high8;
    _out_byte_to_port(0x03d4, 0x0e);
    high8 = _in_byte_from_port(0x03d5);
    _out_byte_to_port(0x03d4, 0x0f);
    return (high8 << 8) | _in_byte_from_port(0x03d5);
}

uint16_t kget_cursor_row_col(void)
{
    uint8_t row, col;
    uint16_t pos = kget_cursor_pos();
    row = pos / 80;
    col = pos % 80;
    return (row << 8) | col;
}

void kroll_screen(void)
{
    memcpy((char*)0xc00b8000, (char*)0xc00b80a0, 3840);
    memset((char*)(0xc00b8000 + 3840), 0x0, 160);
}

static inline void kset_word(uint16_t cursor, uint8_t fst, uint8_t snd)
{
    char* addr  = (char*)0xc00b8000 + (cursor << 1);
    *addr       = fst;
    *(addr + 1) = snd;
}

void kput_char(char ch)
{
    // 取得光标位置
    uint16_t cursor_pos = kget_cursor_pos();
    
    if(ch == '\n' || ch == '\r') // 换行
    {
        cursor_pos += (80 - cursor_pos % 80);
    }
    else if(ch == '\b') // 退格
    {
        if(cursor_pos != 0)
            kset_word(--cursor_pos, (uint8_t)' ', 0x07);
    }
    else if(ch == '\t') // 水平制表四空格，不服憋着
    {
        cursor_pos += (4 - (cursor_pos + 1) % 4);
    }
    else // 普通字符
    {
        kset_word(cursor_pos++, (uint8_t)ch, 0x07);
    }

    // 滚屏
    if(cursor_pos >= 2000)
    {
        kroll_screen();
        cursor_pos = 1920;
    }

    // 更新光标位置
    kset_cursor_pos(cursor_pos);
}

void kput_str(const char *str)
{
    while(*str)
        kput_char(*str++);
}

void kprint_format(const char *fmt, ...)
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
                kput_str(int_buf);
                next_param += 4;
                break;
            case 's':
                kput_str(*(char**)next_param);
                next_param += 4;
                break;
            case '%':
                kput_char('%');
                break;
            case 'c':
                kput_char((char)*(uint32_t*)next_param);
                next_param += 4;
                break;
            }
            ++fmt;
        }
        else
            kput_char(*fmt++);
    }
}

void kset_char(uint16_t pos, char ch)
{
    *(char*)(0xc00b8000 + (pos << 1)) = ch;
}

void kset_attrib(uint16_t pos, char attrib)
{
    *(char*)(0xc00b8000 + (pos << 1) + 1) = attrib;
}
