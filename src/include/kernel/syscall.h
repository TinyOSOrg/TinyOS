#ifndef TINY_OS_SYSCALL_H
#define TINY_OS_SYSCALL_H

#include <lib/stdint.h>

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

    ！！！注意，内联汇编int的时候，一定要用一个变量接受eax的返回值
*/

/* 系统调用初始化，至少在中断初始化后调用 */
void init_syscall();

#endif /* TINY_OS_SYSCALL_H */
