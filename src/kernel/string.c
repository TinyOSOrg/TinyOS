#include <string.h>

size_t _strlen(const char *str)
{
    size_t rt = 0;
    while(str[rt])
        ++rt;
    return rt;
}

void _strcpy(char *dst, const char *src)
{
    while(*dst++ = *src++)
        ;
}

int _strcmp(const char *lhs, const char *rhs)
{
    char L, R;
    while((L = *lhs) && (R = *rhs))
    {
        if(L < R)
            return -1;
        else if(L > R)
            return 1;
        else
            ++lhs, ++rhs;
    }
    return L ? 1 : (R ? -1 : 0);
}

void _uint32_to_str(uint32_t intval, char *buf)
{
    size_t idx = 0;

    if(intval == 0)
    {
        buf[0] = '0';
        buf[1] = '\0';
        return;
    }

    while(intval)
    {
        buf[idx++] = (intval % 10) + '0';
        intval = intval / 10;
    }

    int end = (idx - 1) / 2;
    for(int i = 0;i < end; ++i)
    {
        int tmp = buf[i];
        buf[i] = buf[idx - 1 - i];
        buf[idx - 1 - i] = tmp;
    }

    buf[idx] = '\0';
}
