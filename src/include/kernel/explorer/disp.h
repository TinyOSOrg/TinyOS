#ifndef TINY_OS_EXPLORER_DISP_H
#define TINY_OS_EXPLORER_DISP_H

#include <kernel/explorer/screen.h>

#include <lib/stdint.h>

void init_disp();

void disp_set_cursor(uint8_t x, uint8_t y);

void disp_cursor_end();

void disp_get_cursor(uint8_t *x, uint8_t *y);

void disp_put_char(char ch);

void disp_put_str(const char *str);

void disp_put_line_str(const char *str);

void disp_new_line();

void disp_printf(const char *fmt, ...);

#endif /* TINY_OS_EXPLORER_DISP_H */
