#include <kernel/explorer/cmds.h>
#include <kernel/explorer/disp.h>
#include <kernel/memory.h>

#include <shared/path.h>

#include <lib/sys.h>

void expl_mkdir(filesys_dp_handle dp, const char *working_dir,
                const char *dirname)
{
    // 目标目录缓冲区
    char *namebuf = alloc_ker_page(false);

    // 只能在本分区下创建新文件
    if(is_path_containning_dp(dirname))
        goto FAILED;

    if(!cat_path_s(working_dir, dirname, namebuf, 4096))
        goto FAILED;

    if(make_directory(dp, namebuf) != filesys_opr_success)
        goto FAILED;

    goto SUCCEED;

FAILED:

    disp_printf("Invalid directory name: %s", dirname);
    free_ker_page(namebuf);
    return;

SUCCEED:

    disp_printf("Directory created: %u:%s", dp, namebuf);
    free_ker_page(namebuf);
    return;
}

void expl_rmdir(filesys_dp_handle dp, const char *working_dir,
                const char *dirname)
{
    // 目标目录缓冲区
    char *namebuf = alloc_ker_page(false);

    // 只能在本分区下创建新文件
    if(is_path_containning_dp(dirname))
        goto FAILED;

    if(!cat_path_s(working_dir, dirname, namebuf, 4096))
        goto FAILED;

    if(remove_directory(dp, namebuf) != filesys_opr_success)
        goto FAILED;

    goto SUCCEED;

FAILED:

    disp_printf("Invalid directory name: %s", dirname);
    free_ker_page(namebuf);
    return;

SUCCEED:

    disp_printf("Directory removed: %u:%s", dp, namebuf);
    free_ker_page(namebuf);
    return;
}
