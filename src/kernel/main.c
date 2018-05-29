#include <kernel/explorer/explorer.h>
#include <kernel/kernel.h>
#include <kernel/process/process.h>

#include <kernel/console/print.h>

int main()
{
    init_kernel();

    create_process("explorer", explorer, true);

    while(1)
    {
        do_releasing_thds_procs();
        yield_CPU();
    }
}
