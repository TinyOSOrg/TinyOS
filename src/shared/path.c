#include <shared/path.h>
#include <shared/utility.h>

#include <lib/string.h>

bool is_absolute_path(const char *path)
{
    // 开头是'/'是本分区绝对路径
    if(path[0] == '/')
        return true;
    
    // 第一个'/'的位置
    // assert(slash_pos != 0)
    uint32_t slash_pos = strfind(path, '/', 0);

    // 不含'/'的一定是相对路径
    if(slash_pos == STRING_NPOS)
        return false;

    // 看看第一个名字段是否是分区名
    char dp_end = path[slash_pos - 1];
    if(dp_end == ':' || dp_end == '>')
        return true;
    
    return false;
}

bool is_path_containning_dp(const char *path)
{
    uint32_t slash_pos = strfind(path, '/', 0);
    if(!slash_pos || slash_pos == STRING_NPOS)
        return false;
    char dp_end = path[slash_pos - 1];
    return dp_end == ':' || dp_end == '>';
}

uint32_t get_dp_from_path_s(const char *path,
                            char *dst, uint32_t buf_size)
{
    if(!buf_size)
        return 0;
    
    uint32_t dp_len = strfind(path, '/', 0);
    if(!dp_len || dp_len == STRING_NPOS)
        return 0;
    
    uint32_t cpy_len = MIN(buf_size - 1, dp_len);
    memcpy(dst, path, cpy_len);
    dst[cpy_len] = '\0';

    return cpy_len;
}

const char *skip_dp_in_abs_path(const char *path)
{
    uint32_t beg = strfind(path, '/', 0);
    if(beg == STRING_NPOS)
        return path;
    return path + beg;
}

bool cat_path_s(const char *src, const char *delta,
                char *dst, uint32_t buf_size)
{
    // 如果是绝对路径，直接拷贝
    if(delta[0] == '/')
    {
        if(strlen(delta) >= buf_size)
            return false;
        strcpy(dst, delta);
        return true;
    }
    
    if(strcmp(delta, ".") == 0)
    {
        if(strlen(src) >= buf_size)
            return false;
        strcpy(dst, src);
        return true;
    }

    uint32_t srclen = strlen(src);

    if(strcmp(delta, "..") == 0)
    {
        if(srclen == 1)
        {
            strcpy(dst, src);
            return true;
        }

        uint32_t idx = srclen - 1;
        while(idx > 0 && src[idx] != '/')
            --idx;
        
        if(idx >= buf_size)
            return false;
        
        // 父目录就是根目录
        if(idx == 0)
        {
            if(buf_size < 2)
                return false;
            strcpy(dst, "/");
            return true;
        }
        
        memcpy(dst, src, idx);
        dst[idx] = '\0';

        return true;
    }

    // IMPROVE: 相对路径中不能出现'/'，心情好了倒是可以支持
    if(strfind(delta, '/', 0) != STRING_NPOS)
        return false;

    uint32_t deltalen = strlen(delta);
    
    if(srclen == 1) // 如果当前目录就是根目录，不用拼上'/'
    {
        if(1 + deltalen >= buf_size)
            return false;
        dst[0] = '/';
        strcpy(dst + 1, delta);
    }
    else
    {
        if(srclen + 1 + deltalen >= buf_size)
            return false;
        
        strcpy(dst, src);
        dst[srclen] = '/';
        strcpy(dst + srclen + 1, delta);
    }

    return true;
}
