#include <shared/strhash.h>

/* 算法：ELFHash */
uint32_t strhash(const char *str)
{
    uint32_t ret = 0, t = 0;
    while(*str)
    {
        ret = (ret << 4) + *str++;
        if((t = ret & 0xf0000000) != 0)
            ret =  (ret ^ (t >> 24)) & (~t);
    }
    return ret & 0x7fffffff;
}
