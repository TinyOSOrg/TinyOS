#include <shared/string.h>
#include <shared/sys.h>
#include <shared/utility.h>

#include <lib/input.h>
#include <lib/mem.h>
#include <lib/path.h>

int main(int argc, char *argv[])
{
    if(argc < 2 || argc > 3)
    {
        printf("%LUsage: mf filename [n]");
        return -1;
    }

    bool no_retry = (argc == 3) && strcmp(argv[2], "n") == 0;

    filesys_dp_handle dp;
    char *path = malloc_and_cat_path(argv[0], argv[1], &dp);
    if(!path)
    {
        printf("%LInvalid argument: %s", argv[1]);
        return -1;
    }

    usr_file_handle fp;
    enum filesys_opr_result frt;

    while(true)
    {
        frt = remove_file(dp, path);
        if(frt == filesys_opr_not_found)
            break;
        if(frt != filesys_opr_locked || no_retry)
        {
            printf("%LFailed to remove existing file: %u:%s, err = %u",
                    dp, path, frt);
            return -1;
        }
        yield_cpu();
    }

    while((frt = make_file(dp, path)) != filesys_opr_success)
    {
        if(frt != filesys_opr_locked || no_retry)
        {
            printf("%LFailed to create existing file: %u:%s, err = %u",
                    dp, path, frt);
            return -1;
        }
        yield_cpu();
    }

    while((frt = open_file(dp, path, true, &fp)) != filesys_opr_success)
    {
        if(frt != filesys_opr_locked || no_retry)
        {
            printf("%LFailed to open existing file: %u:%s, err = %u",
                    dp, path, frt);
            return -1;
        }
        yield_cpu();
    }

#define BUF_SIZE 128

    uint32_t fpos = 0;
    char buf[BUF_SIZE]; int idx = 0;

    char ch;
    while((ch = get_char()))
    {
        buf[idx++] = ch;
        if(idx >= BUF_SIZE)
        {
            write_file(fp, fpos, BUF_SIZE, buf);
            fpos += BUF_SIZE;
            idx = 0;
        }
    }

    if(idx)
        write_file(fp, fpos, idx, buf);

    close_file(fp);
    free(path);
}
