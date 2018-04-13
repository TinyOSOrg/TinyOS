#ifndef TINY_OS_SYSCALL_H
#define TINY_OS_SYSCALL_H

#include <lib/intdef.h>

/*
    公用系统调用接口
    合格的签名包括：
        void func(uint32_t number);
        void func(uint32_t number, uint32_t arg0);
        void func(uint32_t number, uint32_t arg0, uint32_t arg1);
        void func(uint32_t number, uint32_t arg0, uint32_t arg1, uint32_t arg2);
*/
void syscall(uint32_t number, uint32_t arg0, uint32_t arg1, uint32_t arg2);

typedef void (*syscall_impl)(void);

/* 注册系统调用实现函数 */
void register_syscall(uint32_t number, syscall_impl func);

#endif /* TINY_OS_SYSCALL_H */
