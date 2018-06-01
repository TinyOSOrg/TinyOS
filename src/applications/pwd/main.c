#include <shared/path.h>
#include <shared/sys.h>

#include <lib/mem.h>

int main(int argc, char *argv[])
{
    expl_new_line();
    printf("Current working directory: %s", argv[0]);
    return 0;
}
