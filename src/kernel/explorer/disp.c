#include <kernel/explorer/disp.h>
#include <kernel/explorer/screen.h>

#include <lib/stdbool.h>
#include <lib/string.h>

static uint8_t cx, cy;

void init_disp()
{
    cx = cy = 0;
}

void disp_set_cursor(uint8_t x, uint8_t y)
{
    cx = x, cy = y;
}

void disp_cursor_end()
{
    cx = SCR_DISP_WIDTH - 1;
    cy = SCR_DISP_HEIGHT - 1;
}

void disp_get_cursor(uint8_t *x, uint8_t *y)
{
    if(x) *x = cx;
    if(y) *y = cy;
}

void disp_put_char(char ch)
{
    if(ch == '\n')
    {
        cx = 0;
        if(cy >= SCR_DISP_HEIGHT - 1)
            disp_roll_screen();
        else
            ++cy;
    }
    else if(ch == '\t')
    {
        for(int t = 4 - (cx % 4); t > 0; --t)
            disp_put_char(' ');
    }
    else
    {
        disp_char(cx, cy, ch);
        if(++cx >= SCR_DISP_WIDTH)
        {
            cx = 0;
            if(cy >= SCR_DISP_HEIGHT - 1)
                disp_roll_screen();
            else
                ++cy;
        }
    }
}

void disp_put_str(const char *str)
{
    while(*str)
        disp_put_char(*str++);
}

void disp_put_line_str(const char *str)
{
    while(*str && cx < SCR_DISP_WIDTH)
        disp_put_char(*str++);
    if(*str)
        disp_put_char(*str);
}

void disp_new_line()
{
    if(cx > 0)
        disp_put_char('\n');
}

void disp_printf(const char *fmt, ...)
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
                disp_put_str(int_buf);
                next_param += 4;
                break;
            case 's':
                disp_put_str(*(char**)next_param);
                next_param += 4;
                break;
            case '%':
                disp_put_char('%');
                break;
            case 'c':
                disp_put_char((char)*(uint32_t*)next_param);
                next_param += 4;
                break;
            case 'l':
                {
                    uint32_to_str(*(uint32_t*)next_param, int_buf);
                    next_param += 4;
                    uint32_t width = *(uint32_t*)next_param;
                    next_param += 4;

                    disp_put_str(int_buf);
                    uint32_t int_len = strlen(int_buf);
                    while(width-- > int_len)
                        disp_put_char(' ');
                }
                break;
            case '\0':
                return;
            }
            ++fmt;
        }
        else
            disp_put_char(*fmt++);
    }
}
