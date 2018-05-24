#ifndef TINY_OS_PROCESS_H
#define TINY_OS_PROCESS_H

#include <kernel/console/con_buf.h>
#include <kernel/memory/vir_mem_man.h>
#include <kernel/process/spinlock.h>
#include <kernel/process/thread.h>
#include <kernel/sysmsg/sysmsg.h>
#include <kernel/sysmsg/sysmsg_src.h>

#include <shared/atrc.h>
#include <shared/filesys/dpt.h>
#include <shared/filesys/filesys.h>
#include <shared/ptrlist.h>

/*
    进程创建和销毁
    这里进程的销毁是通过干掉其中所有的线程实现的，因为最后一个线程退出时会干掉所属进程
    因此进程PCB的销毁是在thread.c中调用的，虽然有些奇怪，我懒得改了……
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

/* 显示缓存大小 */
#define USER_DISPLAYING_BUFFER_SIZE (80 * 25 * 2)

/* 显示缓存起始地址 */
#define USER_DISPLAYING_BUFFER_ADDR \
    (USER_STACK_BITMAP_ADDR - USER_DISPLAYING_BUFFER_SIZE)

/* 用户栈最高地址 */
#define USER_STACK_TOP_ADDR ((uint32_t)0xbffff * (uint32_t)0x1000)

/*
    一个进程最多包含多少线程
    应是32的整数倍
*/
#define MAX_PROCESS_THREADS 32

/* 进程名字最大长度 */
#define PROCESS_NAME_MAX_LENGTH 31

/* 进程前后台状态，影响进程IO方式 */
enum process_interfacing_state
{
    pis_foreground,
    pis_background
};

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

    // 一个进程可以选择某个线程阻塞在消息队列处
    // 若该值非空，则新消息来临时会唤醒该线程，并将该值置为NULL
    struct TCB *sysmsg_blocked_tcb;

    // 内核消息源
    struct sysmsg_source_list sys_msg_srcs;

    // 各种侵入式链表节点

    struct ilist_node processes_node; // 所有进程的链表

    // 文件分配表及其锁
    struct atrc file_table;
    spinlock file_table_lock;

    // 前后台状态
    enum process_interfacing_state pis;

    // 显示缓存
    struct con_buf *disp_buf;
};

/* 进程文件记录 */
struct pcb_file_record
{
    filesys_dp_handle dp;
    file_handle file;
};

/* 进程入口函数签名 */
typedef void (*process_exec_func)();

/*
    进程管理机制初始化
    应在线程管理初始化之后调用
*/
void init_process_man();

/* 创建进程，不解释 */
void create_process(const char *name, process_exec_func func, bool is_PL_0);

/* 在当前进程中追加线程，不解释 */
void add_proc_thread(process_exec_func func);

/* 干掉一个进程 */
void kill_process(struct PCB *pcb);

/* 干掉所有进程（除了bootloader进程） */
void kill_all_processes();

/* 取得当前线程所处进程 */
struct PCB *get_cur_PCB();

/* 获得进程链表，注意bootloader进程不在这里面 */
ilist *get_all_processes();

/*=====================================================================
    下面的东西是给thread.c用的
=====================================================================*/

/* 设置tss中的esp0字段 */
void _set_tss_esp0(uint32_t esp0);

/* 释放进程占用的各种资源，虚拟地址空间和PCB除外 */
void release_process_resources(struct PCB *pcb);

/* 释放进程虚拟地址空间和PCB */
void release_PCB(struct PCB *pcb);

/*=====================================================================
    进程相关系统调用实现
=====================================================================*/

uint32_t syscall_get_cur_PID_impl();

#endif /* TINY_OS_PROCESS_H */
