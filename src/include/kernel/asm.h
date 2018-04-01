#ifndef TINY_OS_IO_PORT
#define TINY_OS_IO_PORT

#include <lib/intdef.h>

static inline uint8_t _in_byte_from_port(uint16_t port)
{
    uint8_t rt;
    asm volatile ("in %1, %0" : "=a" (rt) : "d" (port));
    return rt;
}

static inline void _out_byte_to_port(uint16_t port, uint8_t data)
{
    asm volatile ("out %0, %1" : : "a" (data), "d" (port));
}

static inline void _out_double_byte_to_port(uint16_t port, uint16_t data)
{
    asm volatile ("out %0, %1" : : "a" (data), "d" (port));
}

// 返回从低往高第一个不为0的位的位置
// bits不得为0
static inline uint32_t _find_lowest_nonzero_bit(uint32_t bits)
{
    uint32_t rt;
    asm volatile ("bsf %1, %%eax; movl %%eax, %0" : "=r" (rt) : "r" (bits) : "%eax");
    return rt;
}

// 返回从低往高最后一个不为0的位的位置
// bits不得为0
static inline uint32_t _find_highest_nonzero_bit(uint32_t bits)
{
    uint32_t rt;
    asm volatile ("bsr %1, %%eax; movl %%eax, %0" : "=r" (rt) : "r" (bits) : "%eax");
    return rt;
}

#endif //TINY_OS_IO_PORT
