#ifndef TINY_OS_THREAD_H
#define TINY_OS_THREAD_H

#include <lib/intdef.h>

/* 前向声明：thread control block */
struct TCB;

/*
    用来创建新内核线程时的函数入口
    void*用于参数传递
*/
typedef void (*thread_exec_func)(void*);

/* 初始化内核线程管理系统 */
void init_thread_man(void);

/* 创建新内核线程并加入调度 */
struct TCB *create_thread(thread_exec_func func, void *params);

#endif /* TINY_OS_THREAD_H */
