#include <kernel/assert.h>
#include <kernel/explorer/cmds.h>
#include <kernel/explorer/disp.h>

#include <shared/string.h>

#include <lib/sys.h>

bool expl_cd(filesys_dp_handle *dp, char *cur,
             uint32_t *len, uint32_t max_len,
             const char *dst)
{
    uint32_t dst_len = strlen(dst);
    if(!dst_len)
        return false;

    // 最后一个字符是'/'说明给出的dst是个绝对路径
    if(dst[dst_len - 1] == '/')
    {
        if(dst_len > max_len)
            return false;
        
        // 看看是不是个合法目录
        if(get_child_file_count(*dp, dst, NULL) != filesys_opr_success)
            return false;
        
        strcpy(cur, dst);
        *len = dst_len;
    }
    else
    {

    }

    return true;
}
