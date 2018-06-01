#include <shared/sys.h>

#include <lib/input.h>

int main()
{
    char ch;
    while((ch = get_char()))
        put_char(ch);
    return 0;
}
