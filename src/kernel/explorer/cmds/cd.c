#include <kernel/assert.h>
#include <kernel/explorer/cmds.h>
#include <kernel/explorer/disp.h>

#include <shared/string.h>

#include <lib/sys.h>

/* 将path变换为其父目录，失败时返回false（怎么会） */
static bool to_parent(char *path)
{
    uint32_t len = strlen(path);

    if(!len)
        return false;

    // 根目录的..指向自身，啥都不用做
    if(path[len - 1] == '/')
        return true;
    
    // 查找最右边的'/'所处位置
    uint32_t idx = len - 1;
    while(idx > 0 && path[idx] != '/')
        --idx;
    
    // 父目录就是根目录
    if(idx == 0)
    {
        path[1] = '\0';
        return true;
    }

    path[idx] = '\0';
    return true;
}

bool expl_cd(filesys_dp_handle *dp, char *cur,
             uint32_t *len, uint32_t max_len,
             const char *dst)
{
    uint32_t dst_len = strlen(dst);
    if(!dst_len)
        goto FAILED;

    // 首个路径是'/'说明给出了本分区下的一个绝对路径
    if(dst[0] == '/')
    {
        if(dst_len > max_len)
            goto FAILED;
        
        // 看看是不是个合法目录
        if(get_child_file_count(*dp, dst, NULL) != filesys_opr_success)
            goto FAILED;
        
        strcpy(cur, dst);
        *len = dst_len;

        goto SUCCEED;
    }

    if(strcmp(dst, ".") == 0)
        goto SUCCEED;
    
    if(strcmp(dst, "..") == 0)
    {
        if(!to_parent(cur))
            goto FAILED;
        
        *len = strlen(cur);

        goto SUCCEED;
    }
    
    // 查找'/'，如果没有出现，那么应该是个相对路径
    if(strfind(dst, '/', 0) == STRING_NPOS)
    {
        bool root = (*len == 1);

        if(*len + (root ? 0 : 1) + dst_len > max_len)
            goto FAILED;
        strcat(cur, root ? "" : "/");
        strcat(cur, dst);

        // 看看是不是个合法目录
        if(get_child_file_count(*dp, cur, NULL) != filesys_opr_success)
        {
            cur[*len] = '\0';
            goto FAILED;
        }

        *len = strlen(cur);
        goto SUCCEED;
    }

FAILED:

    disp_printf("Invalid cd destination: %s", dst);
    return false;

SUCCEED:

    disp_printf("Current working directory: %u:%s",
                *dp, cur);
    return true;
}
