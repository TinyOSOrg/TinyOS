#include <kernel/explorer/cmds.h>
#include <kernel/explorer/disp.h>
#include <kernel/filesys/filesys.h>

#include <lib/sys.h>

bool expl_ls(filesys_dp_handle dp, const char *dir)
{
    uint32_t cnt;
    if(get_child_file_count(dp, dir, &cnt) != filesys_opr_success)
    {
        disp_printf("Failed to open directory: %u:%s", dp, dir);
        return false;
    }

    disp_printf("File count: %u", cnt);

    for(uint32_t i = 0; i < cnt; ++i)
    {
        struct syscall_filesys_file_info info;
        if(get_child_file_info(dp, dir, i, &info) != filesys_opr_success)
        {
            disp_printf("Failed to fetch file info: %u:%s:%u", dp, dir, i);
            return false;
        }

        disp_new_line();
        disp_printf("%s  %s", (info.is_dir ? "d" : "r"), info.name);
    }

    return true;
}
