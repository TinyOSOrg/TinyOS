#ifndef TINY_OS_PROCESS_H
#define TINY_OS_PROCESS_H

#include <kernel/memory/vir_mem_man.h>
#include <kernel/process/thread.h>

#include <lib/ptrlist.h>

/*
    进程层面用户虚拟地址空间规划：
        USER_STACK_BITMAP_ADDR~3G可用，3~4G是内核空间
        USER_STACK_BITMAP_ADDR~3G的最后一块区域是线程栈
        USER_STACK_BITMAP_ADDR处用位图记录哪些区域已经用来做栈了
*/

/* 默认用户栈大小：1M */
#define USER_STACK_SIZE 0x100000

/* 用户栈位图起始地址 */
#define USER_STACK_BITMAP_ADDR 0x100000

/* 用户栈最高地址 */
#define USER_STACK_TOP_ADDR ((uint32_t)0xbffff * (uint32_t)0x1000)

/*
    一个进程最多包含多少线程
    应是32的整数倍
*/
#define MAX_PROCESS_THREADS 32

/* 进程名字最大长度 */
#define PROCESS_NAME_MAX_LENGTH 31

/* process control block */
struct PCB
{
    char name[PROCESS_NAME_MAX_LENGTH + 1];

    // 虚拟地址空间句柄
    vir_addr_space *addr_space;

    // 包含哪些线程
    ilist threads_list;

    // 进程空间是否已经初始化
    bool addr_space_inited;

    // 进程编号，初始为0，其他从1开始计
    uint32_t pid;

    // 是否是内核进程，即特权级是否是0
    bool is_PL_0;

    // 各种侵入式链表节点

    struct ilist_node processes_node; // 所有进程的链表
};

/* 进程入口函数签名 */
typedef void (*process_exec_func)(void);

/*
    进程管理机制初始化
    应在线程管理初始化之后调用
*/
void init_process_man(void);

/* 设置tss中的esp0字段 */
void set_tss_esp0(uint32_t esp0);

/* 创建进程，不解释 */
void create_process(const char *name, process_exec_func func, bool is_PL_0);

/* 进程相关系统调用实现 */
uint32_t syscall_get_cur_PID_impl(void);

#endif /* TINY_OS_PROCESS_H */
