#include <print.h>

void print(const char *str)
{
    int16_t pos = 0;
    while(*str)
        _set_char(pos++, *str++);
}

int main(void)
{
    print("minecraft");
    while(1)
        ;
}
