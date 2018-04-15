#ifndef TINY_OS_SYSCALL_H
#define TINY_OS_SYSCALL_H

#include <lib/intdef.h>

/*
    系统调用设计：
        仿照linux，以0x80号中断作为系统调用入口
        以寄存器传参：
            eax为系统调用编号
            ebx为第一个实参
            ecx为第二个实参
            edx为第三个实参

    以下是3个参数的系统调用的参考实现
    对用户进程而言，相关代码当然应该在用户代码段中了，所以在这里没有定义成函数

    // 不妨设系统调用号为N
    uint32_t syscall_with_3_params_in_user_program(uint32_t arg1, uint32_t arg2, uint32_t arg3)
    {
        uint32_t rt;
        asm volatile ("int $0x80;"
                      : "=a" (rt)
                      : "a" (N), "b" (arg1), "c" (arg2), "d" (arg3)
                      : "memory");
        return rt;
    }
*/

/* 系统调用入口数量 */
#define SYSCALL_COUNT 2

/* 一个合法的系统调用应返回void或uint32_t，有0~3个uint32_t大小的参数 */

/*
    取得当前进程ID
    uint32_t impl(void);
*/
#define SYSCALL_GET_PROCESS_ID 0

/*
    控制台操作
    func和arg含义见 kernel/console/console.h
    uint32_t impl(uint32_t func, uint32_t arg);
*/
#define SYSCALL_CONSOLE_OPERATION 1

/* 系统调用初始化，至少在中断初始化后调用 */
void init_syscall(void);

#endif /* TINY_OS_SYSCALL_H */
