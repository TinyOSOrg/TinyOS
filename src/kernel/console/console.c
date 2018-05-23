#include <kernel/console/console.h>
#include <kernel/console/print.h>
#include <kernel/process/process.h>
#include <kernel/process/semaphore.h>

#include <shared/string.h>
#include <shared/syscall/console.h>

static uint32_t (*functions[CONSOLE_SYSCALL_FUNCTION_COUNT])(uint32_t);

static struct con_buf *get_cur_proc_con_buf()
{
    struct PCB *pcb = get_cur_PCB();
    if(pcb->disp_buf)
        return pcb->pis == pis_foreground ? get_sys_con_buf() : pcb->disp_buf;
    return NULL;
}

static uint32_t console_syscall_function_set_char(uint32_t arg)
{
    struct con_buf *buf = get_cur_proc_con_buf();
    if(!buf)
        return 0;

    uint32_t pos = arg >> 16;
    if((arg < 0x20) | (arg > 0x7e) | (pos >= 2000))
        return 0;
    kset_char(buf, arg >> 16, arg & 0xff);
    return 0;
}

static uint32_t console_syscall_function_set_attrib(uint32_t arg)
{
    struct con_buf *buf = get_cur_proc_con_buf();
    if(!buf)
        return 0;

    uint32_t pos = arg >> 16;
    if(pos >= 2000)
        return 0;
    kset_attrib(buf, arg >> 16, arg & 0xff);
    return 0;
}

static uint32_t console_syscall_function_set_char_attrib(uint32_t arg)
{
    struct con_buf *buf = get_cur_proc_con_buf();
    if(!buf)
        return 0;

    uint32_t pos = arg >> 16;
    if(pos >= 2000)
        return 0;
    kset_attrib(buf, pos, arg & 0xff);
    if((arg < 0x20) | (arg > 0x7e))
        return 0;
    kset_char(buf, pos, (arg >> 8) & 0xff);
    return 0;
}

static uint32_t console_syscall_function_clear_screen(uint32_t arg)
{
    struct con_buf *buf = get_cur_proc_con_buf();
    if(!buf)
        return 0;
        
    memset(buf->data, 0x0, CON_BUF_BYTE_SIZE);
    return 0;
}

static uint32_t console_syscall_function_set_cursor(uint32_t arg)
{
    struct con_buf *buf = get_cur_proc_con_buf();
    if(!buf)
        return 0;

    if(arg >= 2000)
        return 0;
    kset_cursor_pos(buf, (uint16_t)arg);
    return 0;
}

static uint32_t console_syscall_function_get_cursor(uint32_t arg)
{
    struct con_buf *buf = get_cur_proc_con_buf();
    if(!buf)
        return 0;

    return kget_cursor_pos(buf);
}

static uint32_t console_syscall_function_put_char(uint32_t arg)
{
    struct con_buf *buf = get_cur_proc_con_buf();
    if(!buf)
        return 0;

    kput_char(buf, (char)arg);
    return 0;
}

static uint32_t console_syscall_function_put_str(uint32_t arg)
{
    struct con_buf *buf = get_cur_proc_con_buf();
    if(!buf)
        return 0;

    const char *p = (const char *)arg;
    while(*p)
        kput_char(buf, *p++);
    return 0;
}

static uint32_t console_syscall_function_roll_screen(uint32_t arg)
{
    struct con_buf *buf = get_cur_proc_con_buf();
    if(!buf)
        return 0;

    kroll_screen(buf);
    return 0;
}

void init_console()
{
    struct con_buf *sys_con_buf = get_sys_con_buf();
    sys_con_buf->cursor = 0;
    init_semaphore(&sys_con_buf->lock, 1);

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
    
    _out_byte_to_port(0x03d4, 0x0e);
    _out_byte_to_port(0x03d5, 2000 >> 8);
    _out_byte_to_port(0x03d4, 0x0f);
    _out_byte_to_port(0x03d5, 2000 & 0xff);
}

struct con_buf *get_sys_con_buf()
{
    return (struct con_buf*)0xc00b8000;
}

uint32_t syscall_console_impl(uint32_t func, uint32_t arg)
{
    if(func >= CONSOLE_SYSCALL_FUNCTION_COUNT)
        return 0;
    struct con_buf *buf = get_cur_proc_con_buf();
    if(!buf)
        return 0;
    semaphore_wait(&buf->lock);
    uint32_t rt = functions[func](arg);
    semaphore_signal(&buf->lock);
    return rt;
}
