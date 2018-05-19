#ifndef TINY_OS_SHARED_CTYPE_H
#define TINY_OS_SHARED_CTYPE_H

static inline bool isdigit(char ch)
{
    return '0' <= ch && ch <= '9';
}

#endif /* TINY_OS_SHARED_CTYPE_H */
