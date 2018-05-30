#include <shared/sys.h>

int main(int argc, char *argv[]);

void _start(void)
{
    main(0, NULL);
    exit_thread();
}
