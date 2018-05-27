#include <kernel/asm.h>
#include <kernel/explorer/explorer.h>
#include <kernel/kernel.h>
#include <kernel/process/process.h>

#include <shared/syscall/common.h>
#include <shared/syscall/sysmsg.h>
#include <shared/sysmsg/kbmsg.h>

#include <lib/sys.h>

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
