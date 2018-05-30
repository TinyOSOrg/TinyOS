#ifndef TINY_OS_SHARED_ASM_H
#define TINY_OS_SHARED_ASM_H

#include <shared/stdint.h>

/*
    返回从低往高第一个不为0的位的位置
    bits不得为0
*/
static inline uint32_t find_lowest_nonzero_bit(uint32_t bits)
{
    uint32_t rt;
    asm volatile ("bsf %1, %%eax; movl %%eax, %0" : "=r" (rt) : "r" (bits) : "%eax");
    return rt;
}

/*
    返回从低往高最后一个不为0的位的位置
    bits不得为0
*/
static inline uint32_t find_highest_nonzero_bit(uint32_t bits)
{
    uint32_t rt;
    asm volatile ("bsr %1, %%eax; movl %%eax, %0" : "=r" (rt) : "r" (bits) : "%eax");
    return rt;
}

/* 原子地将p设置为x并返回x的值 */
static inline uint32_t xchg_u32(volatile uint32_t *p, uint32_t x)  
{    
    asm volatile("xchgl %0, %1"    
               : "=r" (x)    
               : "m" (*p), "0" (x)    
               : "memory");    
    return x;    
}

#endif /* TINY_OS_SHARED_ASM_H */
