#ifndef TINY_OS_EXPLORER_SWITCH_H
#define TINY_OS_EXPLORER_SWITCH_H

#include <kernel/process/process.h>

/*
    进程前后台切换操作
*/

void switch_foreground(struct PCB *src);

void switch_to_foreground(struct PCB *dst);

#endif /* TINY_OS_EXPLORER_SWITCH_H */
