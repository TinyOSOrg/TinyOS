#include <shared/string.h>

size_t strlen(const char *str)
{
    size_t rt = 0;
    while(str[rt])
        ++rt;
    return rt;
}

void strcpy(char *dst, const char *src)
{
    while((*dst++ = *src++))
        ;
}

int strcmp(const char *lhs, const char *rhs)
{
    char L, R;
    while(((L = *lhs) != '\0') & ((R = *rhs) != '\0'))
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

void strcat(char *fst, const char *snd)
{
    while(*fst)
        ++fst;
    strcpy(fst, snd);
}

void uint32_to_str(uint32_t intval, char *buf)
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

    int end = idx / 2;
    for(int i = 0;i < end; ++i)
    {
        int tmp = buf[i];
        buf[i] = buf[idx - 1 - i];
        buf[idx - 1 - i] = tmp;
    }

    buf[idx] = '\0';
}

void memset(char *dst, uint8_t val, size_t byte_size)
{
    for(size_t i = 0;i != byte_size; ++i)
        dst[i] = val;
}

void memcpy(char *dst, const char *src, size_t byte_size)
{
    for(size_t i = 0;i != byte_size; ++i)
        dst[i] = src[i];
}
