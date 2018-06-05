#ifndef TINY_OS_LIB_SPLOCK_H
#define TINY_OS_LIB_SPLOCK_H

#include <shared/stdbool.h>
#include <shared/stdint.h>

typedef uint32_t splock_t;

static inline void sp_init(splock_t *lock)
{
    *lock = 0;
}

bool sp_trylock(splock_t *lock)
{
    uint32_t x = 1;
    asm volatile ("xchgl %0, %1"
        : "=r" (x)
        : "m" (*lock), "0" (x)
        : "memory");
    return !x;
}

void sp_lock(splock_t *lock)
{
    extern void yield_cpu();
    while(({
        uint32_t x = 1;
        asm volatile ("xchgl %0, %1"
                    : "=r" (x)
                    : "m" (*lock), "0" (x)
                    : "memory");
        x;
    }))
        yield_cpu();
}

void sp_unlock(splock_t *lock)
{
    asm volatile("" : : : "memory");
    *lock = 0;
}

#endif /* TINY_OS_LIB_SPLOCK_H */
