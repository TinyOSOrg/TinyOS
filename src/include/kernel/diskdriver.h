#ifndef TINY_OS_DISK_DRIVER_H
#define TINY_OS_DISK_DRIVER_H

#include <shared/intdef.h>

/*
    会直接调用硬盘驱动接口的只有文件系统和缺页中断，缺页中断请求更高

    设置两个标志：硬盘忙标志和缺页标志，上面各能阻塞一个线程

    当有文件系统请求时
        若有硬盘忙标志，就让出CPU，下次再试
        若无硬盘忙标志，有缺页中断标志，提交缺页中断任务，让出CPU，下次唤再试
                     无缺页中断标志，就发送操作命令，置硬盘忙标志，并把自己阻塞上去
        执行后处理
    当有缺页中断请求时
        若有硬盘忙标志，有缺页中断标志，就让出CPU，下次再试
                     无缺页中断标志，就置缺页中断标志，把自己阻塞到缺页标志上
        若无硬盘忙标志，有缺页中断标志，就提交缺页标志上的任务，然后把自己阻塞到缺页中断标志上
                     无缺页中断标志，就置硬盘忙标志，把自己阻塞到硬盘忙标志上
        执行后处理
    提交缺页中断任务：把缺页标志上的任务发给硬盘，将其移交给硬盘忙标志
    硬盘中断处理函数
        若有硬盘忙标志，跳过去 | 清空硬盘忙标志
    后处理的后处理：
        若有缺页中断标志，就提交其缺页中断任务
*/

typedef uint32_t disk_rw_task_type;

#define DISK_RW_TASK_TYPE_READ  0
#define DISK_RW_TASK_TYPE_WRITE 1

/* 一个磁盘读写任务 */
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

void disk_rw_raw(const struct disk_rw_task *task);

void disk_pfrw_raw(const struct disk_rw_task *task);

#endif /* TINY_OS_DISK_DRIVER_H */
