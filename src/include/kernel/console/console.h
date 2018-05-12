#ifndef TINY_OS_CONSOLE_H
#define TINY_OS_CONSOLE_H

#include <shared/intdef.h>

/* 初始化控制台 */
void init_console();

/* 控制台系统调用实现 */
uint32_t syscall_console_impl(uint32_t func, uint32_t arg);

#endif /* TINY_OS_CONSOLE_H */
