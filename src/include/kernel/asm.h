#ifndef TINY_OS_IO_PORT
#define TINY_OS_IO_PORT

#include <shared/asm.h>
#include <shared/stdint.h>

static inline uint8_t _in_byte_from_port(uint16_t port)
{
    uint8_t rt;
    asm volatile ("in %1, %0" : "=a" (rt) : "d" (port));
    return rt;
}

static inline void _in_words_from_port(uint16_t port, void *dst, size_t wcnt)
{
    asm volatile ("cld; rep insw" : "+D" (dst), "+c" (wcnt) : "d" (port) : "memory");
}

static inline void _out_byte_to_port(uint16_t port, uint8_t data)
{
    asm volatile ("out %0, %1" : : "a" (data), "d" (port));
}

static inline void _out_double_byte_to_port(uint16_t port, uint16_t data)
{
    asm volatile ("out %0, %1" : : "a" (data), "d" (port));
}

static inline void _out_words_to_port(uint16_t port, const void *dst, uint32_t wcnt)
{
   asm volatile ("cld; rep outsw" : "+S" (dst), "+c" (wcnt) : "d" (port));
}

static inline void _load_IDT(uint64_t arg)
{
    asm volatile ("lidt %0" : : "m" (arg));
}

static inline void _enable_intr()
{
    asm volatile ("sti");
}

static inline void _disable_intr()
{
    asm volatile ("cli");
}

static inline void _load_cr3(uint32_t val)
{
    asm volatile ("movl %0, %%cr3" : : "r" (val) : "memory");
}

static inline void _refresh_vir_addr_in_TLB(uint32_t vir_addr)
{
    asm volatile ("invlpg %0" : : "m" (vir_addr) : "memory");
}

static inline uint32_t _get_cr2()
{
    uint32_t rt;
    asm volatile ("movl %%cr2, %0" : "=r" (rt));
    return rt;
}

static inline uint32_t _get_cr3()
{
    uint32_t rt;
    asm volatile ("movl %%cr3, %0" : "=r" (rt));
    return rt;
}

static inline uint32_t _get_esp()
{
    uint32_t rt;
    asm volatile ("movl %%esp, %0" : "=r" (rt));
    return rt;
}

static inline uint32_t _get_eflag()
{
    uint32_t rt;
    asm volatile ("pushfl; popl %0" : "=g" (rt));
    return rt;
}

static inline void _load_GDT(uint32_t base, uint32_t size)
{
    uint64_t opr = ((uint64_t)base << 16) | size;
    asm volatile ("lgdt %0" : : "m" (opr));
}

static inline void _ltr(uint16_t sel)
{
    asm volatile ("ltr %w0" : : "r" (sel));
}

#endif /* TINY_OS_IO_PORT */
