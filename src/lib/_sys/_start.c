#include <shared/sys.h>

#include <lib/_sys/_mem.h>

void _start(void)
{
    extern int main(int, char **);

    _init_mem_man();

    main(0, NULL);

    exit_thread();
}
