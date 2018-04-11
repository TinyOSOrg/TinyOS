#include <kernel/asm.h>
#include <kernel/assert.h>
#include <kernel/interrupt.h>
#include <kernel/process/semaphore.h>
#include <kernel/process/thread.h>
#include <kernel/rlist_node_alloc.h>

/*
    现在的信号量实现得很暴力：关中断……
    有闲心再改吧……先做个能跑起来的
*/

void init_semaphore(struct semaphore *s, int32_t init_val)
{
    ASSERT_S(s && init_val > 0);
    s->val = init_val;
    init_rlist(&s->blocked_threads);
}

void semaphore_wait(struct semaphore *s)
{
    intr_state intr_s = fetch_and_disable_intr();

    if(--s->val < 0)
    {
        push_back_rlist(&s->blocked_threads, get_cur_TCB(),
                        kernel_resident_rlist_node_alloc);
        block_cur_thread();
    }

    set_intr_state(intr_s);
}

void semaphore_signal(struct semaphore *s)
{
    intr_state intr_s = fetch_and_disable_intr();

    if(++s->val <= 0)
    {
        ASSERT_S(!is_rlist_empty(&s->blocked_threads));
        struct TCB *th = pop_front_rlist(&s->blocked_threads,
                kernel_resident_rlist_node_dealloc);
        awake_thread(th);
    }

    set_intr_state(intr_s);
}
