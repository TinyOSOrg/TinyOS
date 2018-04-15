#include <kernel/console/console.h>
#include <kernel/console/print.h>
#include <kernel/process/semaphore.h>

static struct semaphore console_mutex;

static uint32_t (*functions[CONSOLE_SYSCALL_FUNCTION_COUNT])(uint32_t);

static uint32_t console_syscall_function_set_char(uint32_t arg)
{
    uint32_t pos = arg >> 16;
    if((arg < 0x20) | (arg > 0x7e) | (pos >= 2000))
        return 0;
    kset_char(arg >> 16, arg & 0xff);
    return 0;
}

static uint32_t console_syscall_function_set_attrib(uint32_t arg)
{
    uint32_t pos = arg >> 16;
    if(pos >= 2000)
        return 0;
    kset_attrib(arg >> 16, arg & 0xff);
    return 0;
}

static uint32_t console_syscall_function_set_char_attrib(uint32_t arg)
{
    uint32_t pos = arg >> 16;
    if(pos >= 2000)
        return 0;
    kset_attrib(pos, arg & 0xff);
    if((arg < 0x20) | (arg > 0x7e))
        return 0;
    kset_char(pos, (arg >> 8) & 0xff);
    return 0;
}

static uint32_t console_syscall_function_clear_screen(uint32_t arg)
{
    uint8_t *chs = (uint8_t*)0xc00b8000;
    for(uint32_t i = 0;i != 2000; ++i)
    {
        chs[i << 1]       = (uint8_t)' ';
        chs[(i << 1) + 1] = 0x07;
    }
    return 0;
}

static uint32_t console_syscall_function_set_cursor(uint32_t arg)
{
    if(arg >= 2000)
        return 0;
    kset_cursor_pos((uint16_t)arg);
    return 0;
}

static uint32_t console_syscall_function_get_cursor(uint32_t arg)
{
    return kget_cursor_pos();
}

static uint32_t console_syscall_function_put_char(uint32_t arg)
{
    kput_char(arg & 0xff);
    return 0;
}

static uint32_t console_syscall_function_put_str(uint32_t arg)
{
    const char *p = (const char *)arg;
    while(*p)
        kput_char(*p++);
    return 0;
}

static uint32_t console_syscall_function_roll_screen(uint32_t arg)
{
    kroll_screen();
    return 0;
}

void init_console(void)
{
    init_semaphore(&console_mutex, 1);

    functions[CONSOLE_SYSCALL_FUNCTION_SET_CHAR] =
        console_syscall_function_set_char;
    functions[CONSOLE_SYSCALL_FUNCTION_SET_ATTRIB] =
        console_syscall_function_set_attrib;
    functions[CONSOLE_SYSCALL_FUNCTION_SET_CHAR_ATTRIB] =
        console_syscall_function_set_char_attrib;
    functions[CONSOLE_SYSCALL_FUNCTION_CLEAR_SCREEN] =
        console_syscall_function_clear_screen;
    functions[CONSOLE_SYSCALL_FUNCTION_SET_CURSOR] =
        console_syscall_function_set_cursor;
    functions[CONSOLE_SYSCALL_FUNCTION_GET_CURSOR] =
        console_syscall_function_get_cursor;
    functions[CONSOLE_SYSCALL_FUNCTION_PUT_CHAR] =
        console_syscall_function_put_char;
    functions[CONSOLE_SYSCALL_FUNCTION_PUT_STR] =
        console_syscall_function_put_str;
    functions[CONSOLE_SYSCALL_FUNCTION_ROLL_SCREEN] =
        console_syscall_function_roll_screen;
}

uint32_t syscall_console_impl(uint32_t func, uint32_t arg)
{
    if(func >= CONSOLE_SYSCALL_FUNCTION_COUNT)
        return 0;
    semaphore_wait(&console_mutex);
    uint32_t rt = functions[func](arg);
    semaphore_signal(&console_mutex);
    return rt;
}
