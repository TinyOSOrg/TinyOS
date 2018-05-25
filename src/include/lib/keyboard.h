#ifndef TINY_OS_LIB_KEYBOARD_H
#define TINY_OS_LIB_KEYBOARD_H

#include <shared/bool.h>
#include <shared/intdef.h>
#include <shared/keycode.h>

#include <shared/sysmsg/kbmsg.h>

/* 某个给定的按键是否处于按压状态 */
bool is_key_pressed(uint8_t kc);

#define is_kbmsg_down(MSG_PTR) \
    ((((struct kbmsg_struct*)(MSG_PTR))->flags & KBMSG_FLAG_UP) == 0)

#define is_kbmsg_up(MSG_PTR) (!is_kbmsg_down(MSG_PTR))

#define get_kbmsg_key(MSG_PTR) ((uint8_t)((struct kbmsg_struct*)(MSG_PTR))->key)

#define get_chmsg_char(MSG_PTR) ((char)((struct kbchar_msg_struct*)(MSG_PTR))->ch)

#endif /* TINY_OS_LIB_KEYBOARD_H */
