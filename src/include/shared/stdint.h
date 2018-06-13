#ifndef TINY_OS_SHARD_STDINT_H
#define TINY_OS_SHARD_STDINT_H

#ifndef TINY_OS_NO_INTDEF

typedef signed char        int8_t;
typedef signed short       int16_t;
typedef signed int         int32_t;
typedef signed long long   int64_t;

typedef unsigned char      uint8_t;
typedef unsigned short     uint16_t;
typedef unsigned int       uint32_t;
typedef unsigned long long uint64_t;

typedef uint32_t size_t;
typedef int32_t  ptrdiff_t;

#define LLONG_MAX 0x7fffffffffffffffLL
#define LLONG_MIN 0x8000000000000000LL

#define INT_MAX   0x7fffffff
#define INT_MIN   0x80000000

#define SHRT_MAX 0x7fff

#define UINT_MAX 0xffffffff

#define UCHAR_MAX 0x255

#define CHAR_BIT 8

#endif /* no TINY_OS_NO_INTDEF */

#ifndef NULL
    #define NULL ((void*)0)
#endif

#endif /* TINY_OS_SHARD_STDINT_H */
