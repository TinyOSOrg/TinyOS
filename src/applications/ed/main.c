#include <shared/sys.h>
#include <lib/path.h>

#include "ed.h"
#include "rf.h"

int main(int argc, char *argv[])
{
    if(argc != 2)
    {
        printf("%LUsage: ed filename");
        return -1;
    }

    // 解析待打开的文件路径
    filesys_dp_handle dp;
    char *path = malloc_and_cat_path(argv[0], argv[1], &dp);
    if(!path)
    {
        printf("%LInvalid filename: %s", argv[1]);
        return -1;
    }

    usr_file_handle fp;
    enum filesys_opr_result frt = open_file(dp, path, false, &fp);

    ed_t *ed = NULL;

    if(frt == filesys_opr_success)
    {
        rf_init(fp);
        ed = new_ed(dp, path, rf_provider);
        close_file(fp);
    }
    else if(frt == filesys_opr_not_found &&
            make_file(dp, path) == filesys_opr_success)
    {
        // 如果找不到，就认为这是在试图创建一个新文件
        ed = new_ed(dp, path, default_ed_text_provider_null);
    }
    else
    {
        printf("%LInvalid filename: %s", argv[1]);
        return -1;
    }

    // 神秘的错误
    if(!ed)
    {
        printf("%LUnknown error");
        return -1;
    }

    // 系统资源申请
    if(!alloc_con_buf())
    {
        printf("%LFailed to allocate screen buffer");
        return -1;
    }
    alloc_fg();
    
    register_key_msg();
    register_char_msg();

    ed_mainloop(ed);

    free_ed(ed);
    return 0;
}
