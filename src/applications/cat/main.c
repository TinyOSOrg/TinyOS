#include <shared/sys.h>
#include <shared/utility.h>

#include <lib/input.h>
#include <lib/mem.h>
#include <lib/path.h>

int main(int argc, char *argv[])
{
    // 接受来自stdin的输入
    if(argc < 2)
    {
        char ch;
        while((ch = get_char()))
            put_char(ch);
        return 0;
    }

    for(int i = 1; i < argc; ++i)
    {
        expl_new_line();

        filesys_dp_handle dp;
        char *path = malloc_and_cat_path(argv[0], argv[i], &dp);
        if(!path)
        {
            printf("Invalid filename: %s", argv[i]);
            return -1;
        }

        usr_file_handle fp;
        if(open_file(dp, path, false, &fp) != filesys_opr_success)
        {
            printf("Failed to open file: %u:%s", dp, path);
            return -1;
        }

        uint32_t file_size = get_file_size(dp);
        char buf[128]; uint32_t fpos = 0;

        while(fpos < file_size)
        {
            uint32_t delta = MIN(128, file_size - fpos);
            read_file(fp, fpos, delta, buf);

            for(int i = 0; i < delta; ++i)
                put_char(buf[i]);
            
            fpos += delta;
        }

        close_file(fp);
        free(path);
    }

    return 0;
}
