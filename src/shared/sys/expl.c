#include <shared/syscall.h>
#include <shared/sys.h>

bool alloc_fg()
{
    return syscall_param0(SYSCALL_EXPL_ALLOC_FOREGROUND);
}

bool free_fg()
{
    return syscall_param0(SYSCALL_EXPL_FREE_FOREGROUND);
}

bool alloc_con_buf()
{
    return syscall_param0(SYSCALL_EXPL_ALLOC_CON_BUF);
}

void put_char_expl(char ch)
{
    uint32_t arg = ch;
    syscall_param1(SYSCALL_EXPL_PUT_CHAR, arg);
}

void expl_new_line()
{
    syscall_param0(SYSCALL_EXPL_NEW_LINE);
}

void pipe_null_char()
{
    syscall_param0(SYSCALL_PIPE_NULL_CHAR);
}
