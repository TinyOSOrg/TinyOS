#ifndef TINY_OS_KB_DRIVER_H
#define TINY_OS_KB_DRIVER_H

#include <kernel/sysmsg/sysmsg.h>

/*
    键盘驱动
    参数定义:
    struct
    {
        uint32_t key
        uint32_t flags
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

#endif /* TINY_OS_KB_DRIVER_H */
