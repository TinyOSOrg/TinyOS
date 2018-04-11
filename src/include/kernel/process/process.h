#ifndef TINY_OS_PROCESS_H
#define TINY_OS_PROCESS_H

#include <kernel/process/thread.h>

/* process control block */
struct PCB;

/*
    进程管理机制初始化
    应在线程管理初始化之后调用
*/
void init_process_man(void);

#endif /* TINY_OS_PROCESS_H */
