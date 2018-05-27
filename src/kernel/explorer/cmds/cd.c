#include <kernel/assert.h>
#include <kernel/explorer/cmds.h>
#include <kernel/explorer/disp.h>

bool expl_cd(filesys_dp_handle *dp, char *cur,
             uint32_t *len, uint32_t max_len,
             const char *arg)
{
    disp_printf("cd!");
    return true;
}
