#include <kernel/explorer/cmds.h>
#include <kernel/explorer/disp.h>
#include <kernel/filesys/filesys.h>

#include <lib/sys.h>

bool expl_ls(filesys_dp_handle dp, const char *dir)
{
    uint32_t rt;
    if(get_child_file_count(dp, dir, &rt) != filesys_opr_success)
    {
        disp_printf("Failed to open directory: %u:%s", dp, dir);
        return false;
    }
    disp_printf("File count: %u", rt);
    return true;
}
