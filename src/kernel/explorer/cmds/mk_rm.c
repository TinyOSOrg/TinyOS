#include <kernel/explorer/cmds.h>
#include <kernel/explorer/disp.h>
#include <kernel/memory.h>

#include <shared/path.h>

#include <lib/sys.h>

static bool expl_file_opr(filesys_dp_handle dp, const char *working_dir,
                          const char *dstname, char *namebuf,
                          enum filesys_opr_result (*opr)(filesys_dp_handle, const char*))
{
    // 只能在本分区下操作
    if(is_path_containning_dp(dstname))
        return false;

    if(!cat_path_s(working_dir, dstname, namebuf, 4096))
        return false;

    if(opr(dp, namebuf) != filesys_opr_success)
        return false;

    return true;
}

void expl_mkdir(filesys_dp_handle dp, const char *working_dir,
                const char *dirname)
{
    // 目标目录缓冲区
    char *namebuf = alloc_ker_page(false);

    if(expl_file_opr(dp, working_dir, dirname, namebuf, make_directory))
        disp_printf("Directory created: %u:%s", dp, namebuf);
    else
        disp_printf("Invalid directory name: %s", dirname);

    free_ker_page(namebuf);
}

void expl_rmdir(filesys_dp_handle dp, const char *working_dir,
                const char *dirname)
{
    // 目标目录缓冲区
    char *namebuf = alloc_ker_page(false);

    if(expl_file_opr(dp, working_dir, dirname, namebuf, remove_directory))
        disp_printf("Directory removed: %u:%s", dp, namebuf);
    else
        disp_printf("Invalid directory name: %s", dirname);

    free_ker_page(namebuf);
}

void expl_rmfile(filesys_dp_handle dp, const char *working_dir,
                 const char *filename)
{
    // 目标目录缓冲区
    char *namebuf = alloc_ker_page(false);

    if(expl_file_opr(dp, working_dir, filename, namebuf, remove_file))
        disp_printf("File removed: %u:%s", dp, namebuf);
    else
        disp_printf("Invalid file name: %s", filename);

    free_ker_page(namebuf);
}
