#ifndef TINY_OS_LIB_CONIO_H
#define TINY_OS_LIB_CONIO_H

#include "intdef.h"

void set_char_row_col(uint8_t row, uint8_t col, char ch);

void set_char_attrib_row_col(uint8_t row, uint8_t col, uint8_t attrib);

void get_cursor_row_col(uint8_t *row, uint8_t *col);

void set_cursor_row_col(uint8_t row, uint8_t col);

#endif /* TINY_OS_LIB_CONIO_H */
