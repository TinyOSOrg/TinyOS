#include <print.h>

void print(const char *str)
{
    while(*str)
        _put_char(*str++);
}

int main(void)
{
    _reset_cursor();
    print("minecraft\nsb\bs");
    while(1)
        ;
}
