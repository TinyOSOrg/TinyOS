#include <kernel/assert.h>
#include <kernel/explorer/cmds.h>
#include <kernel/explorer/disp.h>
#include <kernel/explorer/explorer.h>
#include <kernel/filesys/dpt.h>
#include <kernel/memory.h>

#include <shared/path.h>
#include <shared/string.h>
#include <shared/sys.h>

bool expl_cd(filesys_dp_handle *dp, char *cur,
             uint32_t *len, uint32_t max_len,
             const char *dst)
{
    // 申请一块缓冲区作为dst缓冲
    char *cur_tmp = (char*)alloc_ker_page(false);
    char *dp_name_buf = cur_tmp + max_len;

    // 目标是带分区号的绝对路径
    if(is_path_containning_dp(dst))
    {
        // 获取分区编号
        uint32_t dp_name_len = get_dp_from_path_s(
            dst, dp_name_buf, DP_NAME_BUF_SIZE);

        if(!dp_name_len)
            goto FAILED;

        uint32_t new_dp;

        // 是数字形式的分区号
        if(dp_name_buf[dp_name_len - 1] == ':')
        {
            dp_name_buf[dp_name_len - 1] = '\0';
            if(!str_to_uint32(dp_name_buf, &new_dp))
                goto FAILED;
        }
        else // 是分区名
        {
            ASSERT(dp_name_buf[dp_name_len - 1] == '>');

            dp_name_buf[dp_name_len - 1] = '\0';
            new_dp = get_dp_handle_by_name(dp_name_buf);
            if(new_dp >= DPT_UNIT_COUNT)
                goto FAILED;
        }
        
        if(!cat_path_s(cur, skip_dp_in_abs_path(dst),
                       cur_tmp, max_len))
            goto FAILED;

        if(get_child_file_count(new_dp, cur_tmp, NULL)
            != filesys_opr_success)
            goto FAILED;

        strcpy(cur, cur_tmp);
        *dp = new_dp;

        goto SUCCEED;
    }

    // 目标不带分区，尝试拼接路径
    if(!cat_path_s(cur, dst, cur_tmp, max_len))
        goto FAILED;
    if(get_child_file_count(*dp, cur_tmp, NULL) != filesys_opr_success)
        goto FAILED;
    
    strcpy(cur, cur_tmp);

    goto SUCCEED;

FAILED:

    free_ker_page(cur_tmp);
    disp_printf("Invalid cd destination: %s", dst);
    return false;

SUCCEED:

    free_ker_page(cur_tmp);
    disp_printf("Current working directory: %u:%s",
                *dp, cur);
    return true;
}
