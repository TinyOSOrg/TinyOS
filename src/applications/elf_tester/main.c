#include <lib/sys.h>

int main();

void _start()
{
    main();
    exit_thread();
}

int main()
{
    printf("Hello, elf!\n");

    while(1)
        yield_cpu();
    return 0;
}
