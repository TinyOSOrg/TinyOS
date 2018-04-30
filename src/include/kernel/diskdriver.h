#ifndef TINY_OS_DISK_DRIVER_H
#define TINY_OS_DISK_DRIVER_H

#include <shared/intdef.h>

/*
    硬盘任务调度：
        维护两个阻塞线程队列，一个是普通读写任务，一个是缺页中断任务
        唤醒的时候优先从缺页中断任务中选一个，如果没有缺页任务，再选普通任务
*/

typedef uint8_t disk_rw_task_type;

#define DISK_RW_TASK_TYPE_READ 0
#define DISK_RW_TASK_TYPE_WRITE 1

/* 磁盘读写任务 */
struct disk_rw_task
{
    disk_rw_task_type type;
    size_t sector_base;
    size_t sector_cnt;
    union
    {
        void *read_dst;
        const void *write_src;
    } addr;
};

void init_disk_driver(void);

/*
    执行一个无缓冲硬盘读写
    所以addr不能缺页
*/
void disk_rw_raw(const struct disk_rw_task *task);

/*
    执行一个无缓冲缺页读写
    优先级比一般的读写任务要高
*/
void disk_pfrw_raw(const struct disk_rw_task *task);

#endif /* TINY_OS_DISK_DRIVER_H */
