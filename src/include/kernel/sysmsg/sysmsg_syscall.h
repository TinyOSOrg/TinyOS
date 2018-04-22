#ifndef TINY_OS_PROC_SYSMSG_SYSCALL_H
#define TINY_OS_PROC_SYSMSG_SYSCALL_H

#include <shared/intdef.h>

/* 初始化内核消息系统调用 */
void init_sysmsg_syscall(void);

/* 内核消息系统调用实现 */
uint32_t syscall_sysmsg_impl(uint32_t func, uint32_t arg1, uint32_t arg2);

#endif /* TINY_OS_PROC_SYSMSG_SYSCALL_H */
