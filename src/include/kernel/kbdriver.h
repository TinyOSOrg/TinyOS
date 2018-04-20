#ifndef TINY_OS_KB_DRIVER_H
#define TINY_OS_KB_DRIVER_H

#include <kernel/sysmsg/sysmsg.h>

/*
    键盘驱动
    msg参数含义:
    struct
    {
        uint32_t key
        uint32_t flags
        uint32_t reserved
    }

    flags0 down/up
*/

struct PCB;

struct kbmsg_struct
{
    sysmsg_type type; // 为SYSMSG_TYPE_KEYBOARD
    uint32_t key;
    uint32_t flags;
    uint32_t reserved;
};

#define KBMSG_FLAG_UP 0x1

void init_kb_driver(void);

/*
    订阅键盘消息
    通常由进程自己调用
*/
void subscribe_kb(struct PCB *pcb);

/* 查询某个按键的状态 */
bool is_key_pressed(uint8_t keycode);

/*
    键盘状态查询相关的系统调用
    参数是功能号
*/
uint32_t syscall_keyboard_query_impl(uint32_t func, uint32_t arg);

/*
    功能号：某个按键是否按下
    第一个参数为虚拟键值
*/
#define KEYBOARD_SYSCALL_FUNCTION_IS_KEY_PRESSED 0

#endif /* TINY_OS_KB_DRIVER_H */
