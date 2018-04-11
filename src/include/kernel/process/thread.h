#ifndef TINY_OS_THREAD_H
#define TINY_OS_THREAD_H

#include <lib/intdef.h>

/* 前向声明：thread control block */
struct TCB;

/* 前向声明：process control block */
struct PCB;

/*
    用来创建新内核线程时的函数入口
    void*用于参数传递
*/
typedef void (*thread_exec_func)(void*);

/* 初始化内核线程管理系统 */
void init_thread_man(void);

/* 创建新内核线程并加入调度 */
struct TCB *create_thread(thread_exec_func func, void *params);

/* 取得当前线程的TCB */
struct TCB *get_cur_TCB(void);

/* 阻塞自己，注意系统不会维护阻塞线程 */
void block_cur_thread(void);

/* 唤醒一个blocked线程，将其变为ready */
void awake_thread(struct TCB *tcb);

#endif /* TINY_OS_THREAD_H */
