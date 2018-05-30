#include <shared/sys.h>

#include <lib/_sys/_mem.h>

int main(int argc, char *argv[]);

void _start(void)
{
    _init_mem_man();
    main(0, NULL);
    exit_thread();
}
