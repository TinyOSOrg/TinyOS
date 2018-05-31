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
