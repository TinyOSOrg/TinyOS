#include <print.h>
#include <string.h>

void print(const char *str)
{
    while(*str)
        _put_char(*str++);
}

int main(void)
{
    _set_cursor_pos(2, 0);
    print("minecraft\nsb\bs");
    uint16_t row_col = _get_cursor_row_col();
    _set_cursor_pos(row_col >> 8, (row_col & 0xff) + 5);

    char intstr_buf[10];
    _uint32_to_str(0, intstr_buf);
    print(intstr_buf);
    print("\n");

    print("microsoft");

    _set_char_at_cursor('&');

    while(1)
        ;
}
