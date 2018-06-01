#ifndef TINY_OS_SHARED_PROC_MEM_H
#define TINY_OS_SHARED_PROC_MEM_H

#include <shared/stdint.h>

/* 关于进程虚拟地址空间可用区间的划分 */

/*
    进程初始虚拟地址空间规划：
    addr space
        || 低地址区域：
        ||     0~(USER_STACK_BITMAP_ADDR - 1): 空闲
        ||     USER_STACK_BITMAP_ADDR处为调用栈分配位图
        ||     位图之后紧跟usr_addr_interval
        ||     之后的空间均为空闲
        || 高地址区域：
        ||     elf文件缓存
        ||     进程参数区
        \/     调用栈
*/

/* 用户栈最高地址 */
#define USER_STACK_TOP_ADDR ((uint32_t)0xbffff * (uint32_t)0x1000)

/* 默认用户栈大小：1M */
#define USER_STACK_SIZE 0x100000

/* 用户栈位图起始地址 */
#define USER_STACK_BITMAP_ADDR 0x100000

/*
    一个进程最多包含多少线程
    应是32的整数倍
*/
#define MAX_PROCESS_THREADS 32

/* 有多少个用户栈位图 */
#define USER_THREAD_STACK_BITMAP_COUNT (MAX_PROCESS_THREADS >> 5)

/* 每个参数的buffer size */
#define EXEC_ELF_ARG_BUF_SIZE 256

/* 至多有多少参数 */
#define EXEC_ELF_ARG_MAX_COUNT 24

/* 进程参数区起始地址 */
#define PROC_ARG_ZONE_ADDR \
    (USER_STACK_TOP_ADDR - MAX_PROCESS_THREADS * USER_STACK_SIZE - \
     EXEC_ELF_ARG_MAX_COUNT * EXEC_ELF_ARG_BUF_SIZE - sizeof(uint32_t))

struct usr_addr_interval
{
    size_t beg1, size1;
    size_t beg2, size2;
};

/* 用户地址空间空闲区间存放在哪里 */
#define USR_ADDR_INTERVAL_ADDR \
    (USER_STACK_BITMAP_ADDR + \
     sizeof(uint32_t) * USER_THREAD_STACK_BITMAP_COUNT)

#endif /* TINY_OS_SHARED_PROC_MEM_H */
