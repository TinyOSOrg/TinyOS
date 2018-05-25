#ifndef TINY_OS_LIB_KEYBOARD_H
#define TINY_OS_LIB_KEYBOARD_H

#include <shared/bool.h>
#include <shared/intdef.h>
#include <shared/keycode.h>

/* 某个给定的按键是否处于按压状态 */
bool is_key_pressed(uint8_t kc);

#endif /* TINY_OS_LIB_KEYBOARD_H */
