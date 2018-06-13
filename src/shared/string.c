#include <shared/ctype.h>
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

void strcpy_s(char *dst, const char *src, size_t buf_size)
{
    while(buf_size-- > 1 && (*dst++ = *src++))
        ;
    if(!buf_size)
        *dst = '\0';
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

int strcoll(const char *L, const char *R)
{
    return strcmp(L, R);
}

void strcat(char *fst, const char *snd)
{
    while(*fst)
        ++fst;
    strcpy(fst, snd);
}

uint32_t strfind(const char *str, char c, uint32_t beg)
{
    while(str[beg])
    {
        if(str[beg] == c)
            return beg;
        ++beg;
    }
    return STRING_NPOS;
}

const char *strchr(const char *str, int ch)
{
    size_t offset = strfind(str, (char)ch, 0);
    if(offset != STRING_NPOS)
        return str + offset;
    return NULL;
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

bool str_to_uint32(const char *str, uint32_t *_val)
{
    if(str[0] == '0' && str[1])
        return false;

    uint32_t val = 0;
    while(*str)
    {
        if(!isdigit(*str))
            return false;
        val = 10 * val + (*str++ - '0');
    }

    if(_val)
        *_val = val;
    return true;
}

void memset(void *_dst, uint8_t val, size_t byte_size)
{
    char *dst = (char*)_dst;
    for(size_t i = 0;i != byte_size; ++i)
        dst[i] = val;
}

void memcpy(void *_dst, const void *_src, size_t byte_size)
{
    char *dst = (char*)_dst, *src = (char*)_src;
    for(size_t i = 0;i != byte_size; ++i)
        dst[i] = src[i];
}

int memcmp(const void *L, const void *R, size_t byte_size)
{
    uint8_t *l = (uint8_t*)L, *r = (uint8_t*)R;
    for(size_t i = 0; i < byte_size; ++i)
    {
        if(l[i] < r[i])
            return -1;
        if(l[i] > r[i])
            return 1;
    }
    return 0;
}

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
