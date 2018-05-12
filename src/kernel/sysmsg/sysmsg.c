#include <kernel/assert.h>
#include <kernel/memory.h>
#include <kernel/process/process.h>
#include <kernel/sysmsg/sysmsg.h>

#include <shared/string.h>
#include <shared/sysmsg/common.h>
#include <shared/utility.h>

/* 消息队列容量 */
#define SYSMSG_QUEUE_MAX_SIZE (4096 / sizeof(struct sysmsg))

void init_sysmsg()
{
    // 暂时没啥好做的……
}

void init_sysmsg_queue(struct sysmsg_queue *queue)
{
    ASSERT_S(queue);
    queue->msgs = (struct sysmsg*)alloc_ker_page(false);
    queue->head = 0;
    queue->tail = 0;
    queue->size = 0;
}

void destroy_sysmsg_queue(struct sysmsg_queue *queue)
{
    ASSERT_S(queue);
    free_ker_page(queue->msgs);
}

bool send_sysmsg(struct sysmsg_queue *queue, const struct sysmsg *msg)
{
    ASSERT_S(queue && msg);

    if(queue->size >= SYSMSG_QUEUE_MAX_SIZE)
        return false;
    
    memcpy((char*)&queue->msgs[queue->tail], (char*)msg, sizeof(struct sysmsg));
    queue->tail = (queue->tail + 1) % SYSMSG_QUEUE_MAX_SIZE;
    ++queue->size;

    // 唤醒被消息队列阻塞的线程
    struct PCB *pcb = GET_STRUCT_FROM_MEMBER(struct PCB, sys_msgs, queue);
    if(pcb->sysmsg_blocked_tcb)
    {
        awake_thread(pcb->sysmsg_blocked_tcb);
        pcb->sysmsg_blocked_tcb = NULL;
    }

    return true;
}

bool is_sysmsg_queue_empty(const struct sysmsg_queue *queue)
{
    ASSERT_S(queue);
    return queue->size == 0;
}

bool is_sysmsg_queue_full(const struct sysmsg_queue *queue)
{
    ASSERT_S(queue);
    return queue->size >= SYSMSG_QUEUE_MAX_SIZE;
}

const struct sysmsg *fetch_sysmsg_queue_front(const struct sysmsg_queue *queue)
{
    ASSERT_S(queue);
    return queue->size == 0 ? NULL : &queue->msgs[queue->head];
}

bool pop_sysmsg_queue_front(struct sysmsg_queue *queue, struct sysmsg *output)
{
    ASSERT_S(queue && output);

    if(queue->size == 0)
        return false;

    memcpy((char*)output, (char*)&queue->msgs[queue->head], sizeof(struct sysmsg));
    queue->head = (queue->head + 1) % SYSMSG_QUEUE_MAX_SIZE;
    --queue->size;

    return true;
}
