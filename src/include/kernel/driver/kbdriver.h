#ifndef TINY_OS_KB_DRIVER_H
#define TINY_OS_KB_DRIVER_H

#include <kernel/sysmsg/sysmsg.h>

/*
    键盘驱动
*/

struct PCB;

/* 键盘驱动初始化 */
void init_kb_driver();

/*
    订阅键盘消息
    通常由进程自己调用
*/
void subscribe_kb(struct PCB *pcb);

/*
    订阅字符输入消息
*/
void subscribe_char(struct PCB *pcb);

/* 查询某个按键的状态 */
bool kis_key_pressed(uint8_t keycode);

/*
    键盘状态查询相关的系统调用
    参数是功能号，见shared/syscall/keyboard
*/
uint32_t syscall_keyboard_query_impl(uint32_t func, uint32_t arg);

#endif /* TINY_OS_KB_DRIVER_H */
