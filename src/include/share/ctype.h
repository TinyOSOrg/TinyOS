#ifndef TINY_OS_SHARD_CTYPE_H
#define TINY_OS_SHARD_CTYPE_H

#include <shared/stdbool.h>

static inline bool isdigit(char ch)
{
    return '0' <= ch && ch <= '9';
}

static inline bool isspace(char ch)
{
    // 暂时只管这三个……
    return ch == ' ' || ch == '\n' || ch == '\t';
}

#endif /* TINY_OS_SHARD_CTYPE_H */
