#ifndef TINY_OS_SHARED_SYSCALL_COMMON_H
#define TINY_OS_SHARED_SYSCALL_COMMON_H

/* 系统调用入口数量 */
#define SYSCALL_COUNT 4

/* 一个合法的系统调用应返回void或uint32_t，有0~3个uint32_t大小的参数 */

/*
    取得当前进程ID
    uint32_t impl();
*/
#define SYSCALL_GET_PROCESS_ID 0

/*
    控制台操作
    func和arg含义见 kernel/console/console.h
    uint32_t impl(uint32_t func, uint32_t arg);
*/
#define SYSCALL_CONSOLE_OPERATION 1

/*
    系统消息队列操作
    参数含义见 kernel/sysmsg/proc_sysmsg_syscall.h
    uint32_t impl(uint32_t func, uint32_t arg1, uint32_t arg2);
*/
#define SYSCALL_SYSMSG_OPERATION 2

/*
    键盘查询
    参数含义见kernel/kbdriver.h
    uint32_t impl(uint32_t func, uint32_t arg);
*/
#define SYSCALL_KEYBOARD_QUERY 3

/* AFS相关 */

#define SYSCALL_AFS_CREATE_REGULAR   4
#define SYSCALL_AFS_CREATE_DIRECTORY 5

#define SYSCALL_AFS_EXISTS           6

#define SYSCALL_AFS_REMOVE_REGULAR   7
#define SYSCALL_AFS_REMOVE_DIRECTORY 8

#define SYSCALL_AFS_OPEN_REGULAR     9
#define SYSCALL_AFS_OPEN_DIRECTORY   10

#define SYSCALL_AFS_CLOSE            11

#endif /* TINY_OS_SHARED_SYSCALL_COMMON_H */
