#ifndef TINY_OS_SEMAPHORE_H
#define TINY_OS_SEMAPHORE_H

#include <shared/intdef.h>
#include <shared/ptrlist.h>

/*
    内核信号量，提供PV原语
    P：
        if(--value < 0)
            block();
    V：
        if(++value <= 0)
            awake_one();
*/

struct semaphore
{
    volatile int32_t val;
    rlist blocked_threads;
};

/* 信号量初始化 */
void init_semaphore(struct semaphore *s, int32_t init_val);

/* P */
void semaphore_wait(struct semaphore *s);

/* V */
void semaphore_signal(struct semaphore *s);

#endif /* TINY_OS_SEMAPHORE_H */
