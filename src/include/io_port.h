#ifndef TINY_OS_IO_PORT
#define TINY_OS_IO_PORT

#include "integer.h"

inline uint8_t _in_byte_from_port(uint16_t port)
{
    uint8_t rt;
    asm volatile ("in %1, %0" : "=a" (rt) : "d" (port));
    return rt;
}

inline void _out_byte_to_port(uint16_t port, uint8_t data)
{
    asm volatile ("out %0, %1" : : "a" (data), "d" (port));
}

inline void _out_double_byte_to_port(uint16_t port, uint16_t data)
{
    asm volatile("out %0, %1" : : "a" (data), "d" (port));
}

#endif //TINY_OS_IO_PORT
