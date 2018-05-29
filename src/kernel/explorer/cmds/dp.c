#include <kernel/explorer/cmds.h>
#include <kernel/explorer/disp.h>
#include <kernel/filesys/dpt.h>

void expl_dp(filesys_dp_handle dp)
{
    struct dpt_unit *u = get_dpt_unit(dp);
    if(!u)
    {
        disp_printf("Invalid working dp: %u", dp);
        return;
    }

    disp_printf("Current working dp: (%u)%s", dp, u->name);
}
