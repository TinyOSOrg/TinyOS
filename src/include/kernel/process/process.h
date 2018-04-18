#ifndef TINY_OS_PROCESS_H
#define TINY_OS_PROCESS_H

#include <kernel/memory/vir_mem_man.h>
#include <kernel/process/thread.h>
#include <kernel/sysmsg/sysmsg.h>
#include <kernel/sysmsg/sysmsg_src.h>

#include <lib/ptrlist.h>

/*
    进程创建和销毁
    这里进程的销毁是通过干掉其中所有的线程实现的，因为最后一个线程退出时会干掉所属进程
    因此进程PCB的销毁是在thread.c中实现的，虽然有些奇怪，我懒得改了……
*/

/*
    进程层面用户虚拟地址空间规划：
        USER_STACK_BITMAP_ADDR~3G可用，3~4G是内核空间
        USER_STACK_BITMAP_ADDR~3G的最后一块区域是线程栈
        USER_STACK_BITMAP_ADDR处用位图记录哪些区域已经用来做栈了
*/

/* 最大进程数量（包括内核初始进程） */
#define MAX_PROCESS_COUNT (MAX_VIR_ADDR_SPACE_COUNT + 1)

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

    // 内核消息队列
    struct sysmsg_queue sys_msgs;

    // 内核消息源
    struct sysmsg_source_list sys_msg_srcs;

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

/* 创建进程，不解释 */
void create_process(const char *name, process_exec_func func, bool is_PL_0);

/* 进程相关系统调用实现 */
uint32_t syscall_get_cur_PID_impl(void);

/* 干掉一个进程 */
void kill_process(struct PCB *pcb);

/*=====================================================================
    下面的东西是给thread.c用的
=====================================================================*/

/* 设置tss中的esp0字段 */
void _set_tss_esp0(uint32_t esp0);

/* PCB自由链表 */
void _add_PCB_mem(struct PCB *pcb);

/* 清空PID到PCB映射中的某一项 */
void _clr_PID_to_PCB(uint32_t pid);

#endif /* TINY_OS_PROCESS_H */
