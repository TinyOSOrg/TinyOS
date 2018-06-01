#include <kernel/assert.h>
#include <kernel/execelf/execelf.h>
#include <kernel/explorer/cmds.h>
#include <kernel/explorer/disp.h>
#include <kernel/filesys/dpt.h>
#include <kernel/interrupt.h>
#include <kernel/memory.h>

#include <shared/path.h>
#include <shared/string.h>

static bool expl_exec_impl(filesys_dp_handle dp, const char *working_dir,
                           const char *dst, const char **args, uint32_t args_cnt, bool pa,
                           uint32_t *_pid)
{
    ASSERT(args && args_cnt >= 1);
    const char *proc_name = args[0];

    const char *arg_working_dir = working_dir;
    if(pa)
        working_dir = "/apps";

    if(strcmp(proc_name, "`") == 0)
        proc_name = dst;

    // 路径缓存
    char *path_buf = (char*)alloc_ker_page(false);

#define DP_STR_BUF_SIZE 2048
#define DP_STR_OFFSET (4096 - DP_STR_BUF_SIZE)

    uint32_t dst_dp;
    if(!cat_path_ex_s(dp, working_dir, dst,
                      &dst_dp, path_buf, DP_STR_OFFSET))
        goto FAILED;

    uint32_t pid;

    char *arg_cur_buf = path_buf + DP_STR_OFFSET;
    uint32_to_str(dst_dp, arg_cur_buf);
    strcat(arg_cur_buf, ":");
    strcat(arg_cur_buf, arg_working_dir);
    args[0] = arg_cur_buf;

    enum exec_elf_result rt = exec_elf(
        proc_name, dst_dp, path_buf, false, args_cnt, args, &pid);
    
    if(rt != exec_elf_success)
        goto FAILED;

    if(_pid)
        *_pid = pid;
    
    free_ker_page(path_buf);
    return true;

FAILED:

    free_ker_page(path_buf);
    if(!pa)
        return expl_exec_impl(0, working_dir, dst, args, args_cnt, true, _pid);
    return false;
}

bool expl_exec(filesys_dp_handle dp, const char *working_dir,
               const char *dst, const char **args, uint32_t args_cnt,
               uint32_t *_pid)
{
    return expl_exec_impl(dp, working_dir, dst, args, args_cnt, false, _pid);
}
