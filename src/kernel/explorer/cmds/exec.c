#include <kernel/assert.h>
#include <kernel/execelf/execelf.h>
#include <kernel/explorer/cmds.h>
#include <kernel/explorer/disp.h>
#include <kernel/filesys/dpt.h>
#include <kernel/memory.h>

#include <shared/path.h>
#include <shared/string.h>

void expl_exec(filesys_dp_handle dp, const char *working_dir,
               const char **_args, uint32_t args_cnt)
{
    ASSERT(_args && args_cnt >= 2);
    const char *dst = _args[0], *proc_name = _args[1], **args = _args + 1;

    // 路径缓存
    char *path_buf = (char*)alloc_ker_page(false);

#define DP_STR_BUF_SIZE 2048
#define DP_STR_OFFSET (4096 - DP_STR_BUF_SIZE)

    uint32_t dst_dp;
    if(!cat_path_ex_s(dp, working_dir, dst, &dst_dp, path_buf, DP_STR_OFFSET))
    {
        disp_new_line();
        disp_printf("Invalid elf name");
        goto EXIT;
    }

    uint32_t pid;
    if(strcmp(proc_name, "`") == 0)
        proc_name = dst;
        
    char *arg_cur_buf = path_buf + DP_STR_OFFSET;
    uint32_to_str(dst_dp, arg_cur_buf);
    strcat(arg_cur_buf, ":");
    strcat(arg_cur_buf, working_dir);
    args[0] = arg_cur_buf;

    enum exec_elf_result rt = exec_elf(
        proc_name, dst_dp, path_buf, false, args_cnt - 1, args, &pid);
    
    disp_new_line();
    switch(rt)
    {
    case exec_elf_file_error:
        disp_printf("File error");
        break;
    case exec_elf_invalid_elf:
        disp_printf("Invalid elf");
        break;
    case exec_elf_success:
        disp_printf("Proc created: %u, %s", pid, proc_name);
        break;
    }

EXIT:

    free_ker_page(path_buf);
}
