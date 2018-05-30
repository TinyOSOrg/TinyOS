#include <kernel/explorer/cmds.h>
#include <kernel/explorer/disp.h>
#include <kernel/memory.h>

#include <shared/path.h>
#include <shared/sys.h>

static bool expl_file_opr(filesys_dp_handle dp, const char *working_dir,
                          const char *dstname, char *namebuf,
                          enum filesys_opr_result (*opr)(filesys_dp_handle, const char*))
{
    filesys_dp_handle dst_dp;
    if(!cat_path_ex_s(dp, working_dir, dstname, &dst_dp, namebuf, 4096))
        return false;

    enum filesys_opr_result rt = opr(dst_dp, namebuf);
    if(rt != filesys_opr_success)
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
