#ifndef TINY_OS_LIB_KEYBOARD_H
#define TINY_OS_LIB_KEYBOARD_H

#include <shared/sysmsg.h>

/* 虚拟键值定义 */

/*
    字母：ASCII码（大写）
    数字：'0'~'9'
*/

#define VK_NULL    255

#define VK_ESCAPE  0

#define VK_F1      1
#define VK_F2      2
#define VK_F3      3
#define VK_F4      4
#define VK_F5      5
#define VK_F6      6
#define VK_F7      7
#define VK_F8      8
#define VK_F9      9
#define VK_F10     10
#define VK_F11     11
#define VK_F12     12

#define VK_SIM     13 /* `~ */
#define VK_MINUS   14 /* -_ */
#define VK_EQUAL   15 /* =+ */

#define VK_TAB     16
#define VK_BS      17 /* backspace */
#define VK_ENTER   18
#define VK_SPACE   19

#define VK_LBRAC   20 /* [{ */
#define VK_RBRAC   21 /* ]} */
#define VK_BACKSL  22 /* \| */
#define VK_SEMICOL 23 /* ;: */
#define VK_QOT     24 /* '" */
#define VK_COMMA   25 /* ,< */
#define VK_POINT   26 /* .> */
#define VK_DIV     27 /* /? */

#define VK_LSHIFT  28
#define VK_RSHIFT  29
#define VK_LCTRL   30
#define VK_RCTRL   31
#define VK_LALT    32
#define VK_RALT    33

#define VK_CAPS    34 /* caps lock */

#define VK_NUMLOCK 35
#define VK_SCRLOCK 36

#define VK_PAD_HOME    37
#define VK_PAD_UP      38
#define VK_PAD_DOWN    39
#define VK_PAD_LEFT    40
#define VK_PAD_RIGHT   41
#define VK_PAD_PGUP    42
#define VK_PAD_PGDOWN  43
#define VK_PAD_FIVE    44
#define VK_PAD_END     45
#define VK_PAD_INS     46
#define VK_PAD_DEL     47
#define VK_PAD_PLUS    48
#define VK_PAD_ENTER   49
#define VK_PAD_MINUS   50
#define VK_PAD_DIV     51

#define VK_INSERT      52
#define VK_HOME        53
#define VK_PGUP        54
#define VK_PGDOWN      55
#define VK_DELETE      56
#define VK_END         57
#define VK_LEFT        58
#define VK_RIGHT       59
#define VK_UP          60
#define VK_DOWN        61

/*
    系统调用功能号：某个按键是否按下
    第一个参数为虚拟键值
*/
#define KEYBOARD_SYSCALL_FUNCTION_IS_KEY_PRESSED 0

/*================================================================================
    按键消息和字符消息
================================================================================*/

/*
    msg参数含义:
    struct
    {
        uint32_t key
        uint32_t flags
        uint32_t reserved
    }

    flags0 down/up
*/
struct kbmsg_struct
{
    sysmsg_type type; // 为SYSMSG_TYPE_KEYBOARD
    uint32_t key;
    uint32_t flags;
    uint32_t reserved;
};

/*
    char消息
    struct
    {
        uint32_t char
    }
*/
struct kbchar_msg_struct
{
    sysmsg_type type; // 为SYSMSG_TYPE_CHAR
    uint32_t ch;
    uint32_t reserved1;
    uint32_t reserved2;
};

#define KBMSG_FLAG_UP 0x1

#endif /* TINY_OS_LIB_KEYBOARD_H */
