#ifndef TINY_OS_PROCESS_SPIN_LOCK_H
#define TINY_OS_PROCESS_SPIN_LOCK_H

#include <kernel/asm.h>
#include <kernel/process/thread.h>

#include <lib/stdint.h>

/* 自旋锁实现的互斥量，不解释 */

// typedef volatile uint32_t spinlock; in thread.h

static inline void init_spinlock(spinlock *lock)
{
    *lock = 0;
}

/* 在单核时，自旋锁的等待循环完全是时间浪费（并不能避免线程切换开销），所以这里的lock在发生自旋时会yield CPU */
static inline void spinlock_lock(spinlock *lock)
{
    while(xchg_u32(lock, 1))
        yield_CPU();
}

static inline void spinlock_unlock(spinlock *lock)
{
    __asm__ __volatile__("" : : : "memory");
    *lock = 0;
}

#endif /* TINY_OS_PROCESS_SPIN_LOCK_H */
