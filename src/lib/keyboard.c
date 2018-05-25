#include <shared/syscall/common.h>
#include <shared/syscall/keyboard.h>
#include <shared/syscall/sysmsg.h>

#include <lib/keyboard.h>

bool is_key_pressed(uint8_t kc)
{
    return syscall_param2(SYSCALL_KEYBOARD_QUERY,
                KEYBOARD_SYSCALL_FUNCTION_IS_KEY_PRESSED, kc) != 0;
}
