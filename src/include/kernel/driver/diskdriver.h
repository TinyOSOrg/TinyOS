#ifndef TINY_OS_DISK_DRIVER_H
#define TINY_OS_DISK_DRIVER_H

#include <shared/intdef.h>

/*
    硬盘任务调度：
        维护一个阻塞线程队列，唤醒的时候从中选一个
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

void init_disk_driver();

/*
    执行一个无缓冲硬盘读写
    所以addr不能缺页
*/
void disk_rw_raw(const struct disk_rw_task *task);

#define disk_read(SEC, CNT, DATA) \
    do { \
        struct disk_rw_task task = \
        { \
            .type          = DISK_RW_TASK_TYPE_READ, \
            .sector_base   = (SEC), \
            .sector_cnt    = (CNT), \
            .addr.read_dst = (DATA) \
        }; \
        disk_rw_raw(&task); \
    } while(0)

#define disk_write(SEC, CNT, DATA) \
    do { \
        struct disk_rw_task task = \
        { \
            .type           = DISK_RW_TASK_TYPE_WRITE, \
            .sector_base    = (SEC), \
            .sector_cnt     = (CNT), \
            .addr.write_src = (DATA) \
        }; \
        disk_rw_raw(&task); \
    } while(0)

#endif /* TINY_OS_DISK_DRIVER_H */
