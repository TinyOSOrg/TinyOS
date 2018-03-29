#include <print.h>

void _set_char(int16_t idx, char ch)
{
    *((char*)0xb8000 + (idx << 1)) = ch;
}
