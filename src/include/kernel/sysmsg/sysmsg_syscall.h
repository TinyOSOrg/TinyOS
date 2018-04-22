#ifndef TINY_OS_PROC_SYSMSG_SYSCALL_H
#define TINY_OS_PROC_SYSMSG_SYSCALL_H

#include <lib/intdef.h>

/*
    每个进程有自己的系统消息队列
    下面是于此相关的系统调用约定
*/

/*
    各种消息队列操作共用同一个系统调用号，所以第一个参数是功能号
    后面的参数含义由功能号定义
*/

/* 本进程系统消息队列是否为空，无参数 */
#define SYSMSG_SYSCALL_FUNCTION_IS_EMPTY              0

/*
    第一个参数：对消息的处理方式，可按位或，见下面的各种PEEK_OPERATION
    第二个参数：输出缓冲区在用户虚拟地址空间中的地址
    返回值：是否peek到消息
*/
#define SYSMSG_SYSCALL_FUNCTION_PEEK_MSG              1

#define SYSMSG_SYSCALL_PEEK_OPERATION_REMOVE 1

/* 注册接受键盘消息，无参数 */
#define SYSMSG_SYSCALL_FUNCTION_REGISTER_KEYBOARD_MSG 2

/* 注册接受字符消息，无参数 */
#define SYSMSG_SYSCALL_FUNCTION_REGISTER_CHAR_MSG     3

/* 将当前线程阻塞在进程消息队列中，无参数 */
#define SYSMSG_SYSCALL_FUNCTION_BLOCK_ONTO_SYSMSG     4

#define SYSMSG_SYSCALL_FUNCTION_COUNT 5

/* 初始化内核消息系统调用 */
void init_sysmsg_syscall(void);

/* 内核消息系统调用实现 */
uint32_t syscall_sysmsg_impl(uint32_t func, uint32_t arg1, uint32_t arg2);

#endif /* TINY_OS_PROC_SYSMSG_SYSCALL_H */
