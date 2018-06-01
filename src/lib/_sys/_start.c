#include <shared/proc_mem.h>
#include <shared/sys.h>

#include <lib/_sys/_mem.h>
#include <lib/input.h>

void _start(void)
{
    extern int main(int, char **);

    _init_input();
    _init_mem_man();

    uint32_t parg = PROC_ARG_ZONE_ADDR;
    char *argv[EXEC_ELF_ARG_MAX_COUNT];

    uint32_t argc = *(uint32_t*)parg;
    parg += sizeof(uint32_t);

    for(uint32_t i = 0; i < argc; ++i)
        argv[i] = (char*)(parg + i * EXEC_ELF_ARG_BUF_SIZE);

    main(argc, argv);

    pipe_null_char();
    exit_thread();
}
