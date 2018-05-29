#include <kernel/assert.h>
#include <kernel/execelf/execelf.h>
#include <kernel/explorer/cmds.h>
#include <kernel/explorer/disp.h>
#include <kernel/filesys/dpt.h>
#include <kernel/memory.h>

#include <shared/path.h>

#include <lib/string.h>

void expl_exec(filesys_dp_handle dp, const char *working_dir,
               const char **_args, uint32_t args_cnt)
{
    ASSERT_S(_args && args_cnt >= 2);
    const char *dst = _args[0], *proc_name = _args[1], **args = _args + 2;

    // 路径缓存
    char *path_buf = (char*)alloc_ker_page(false);
    char *dp_name_buf = path_buf + (4096 / 2);

    uint32_t dst_dp;

    if(is_path_containning_dp(dst))
    {
        uint32_t dp_name_len = get_dp_from_path_s(
            dst, dp_name_buf, (4096 / 2));
        
        if(!dp_name_len)
        {
            disp_new_line();
            disp_printf("Invalid dp name");
            goto EXIT;
        }

        // 是数字形式的分区号
        if(dp_name_buf[dp_name_len - 1] == ':')
        {
            dp_name_buf[dp_name_len - 1] = '\0';
            if(!str_to_uint32(dp_name_buf, &dst_dp))
            {
                disp_new_line();
                disp_printf("Invalid dp index");
                goto EXIT;
            }
        }
        else // 是分区名
        {
            ASSERT_S(dp_name_buf[dp_name_len - 1] == '>');

            dp_name_buf[dp_name_len - 1] = '\0';
            dst_dp = get_dp_handle_by_name(dp_name_buf);
            if(dst_dp >= DPT_UNIT_COUNT)
            {
                disp_new_line();
                disp_printf("Invalid dp name");
                goto EXIT;
            }
        }

        if(!cat_path_s(working_dir, skip_dp_in_abs_path(dst),
                       path_buf, (4096 / 2)))
        {
            disp_new_line();
            disp_printf("Invalid elf name");
            goto EXIT;
        }
    }
    else
    {
        if(!cat_path_s(working_dir, dst,
                       path_buf, (4096 / 2)))
        {
            disp_new_line();
            disp_printf("Invalid elf name");
            goto EXIT;
        }

        dst_dp = dp;
    }

    uint32_t pid;
    enum exec_elf_result rt = exec_elf(
        proc_name, dst_dp, path_buf, false, args_cnt - 2, args, &pid);
    
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
