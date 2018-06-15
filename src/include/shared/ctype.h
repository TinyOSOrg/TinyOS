#ifndef TINY_OS_SHARD_CTYPE_H
#define TINY_OS_SHARD_CTYPE_H

#include <shared/stdbool.h>

static inline bool isprint(char ch)
{
    return 32 <= ch && ch <= 126;
}

static inline bool isalpha(char ch)
{
    return ('a' <= ch && ch <= 'z') ||
           ('A' <= ch && ch <= 'Z');
}

static inline bool isdigit(char ch)
{
    return '0' <= ch && ch <= '9';
}

static inline bool isalnum(char ch)
{
    return isalpha(ch) || isdigit(ch);
}

static inline bool isspace(char ch)
{
    // 暂时只管这三个……
    return ch == ' ' || ch == '\n' || ch == '\t';
}

static inline char to_upper(char ch)
{
    if('a' <= ch && ch <= 'z')
        return ch - 'a' + 'A';
    return ch;
}

static inline char to_lower(char ch)
{
    if('A' <= ch && ch <= 'Z')
        return ch - 'A' + 'a';
    return ch;
}

#endif /* TINY_OS_SHARD_CTYPE_H */
