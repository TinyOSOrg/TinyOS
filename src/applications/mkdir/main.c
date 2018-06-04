#include <shared/string.h>
#include <shared/sys.h>

#include <lib/mem.h>
#include <lib/path.h>

/*
    path应具有以下形式：
        (/blabla)+
    一段一段地处理
*/
void make_dir_chain(filesys_dp_handle dp, const char *path)
{
    char *pp = malloc(strlen(path) + 1);
    size_t ip = 0;
    while(path[ip])
    {
        pp[ip] = path[ip];
        ++ip;
        while(path[ip] && path[ip] != '/')
        {
            pp[ip] = path[ip];
            ++ip;
        }
        pp[ip] = '\0';

        make_directory(dp, pp);
    }
    free(pp);
}

int main(int argc, char *argv[])
{
    if(argc < 2)
    {
        printf("%LUsage: mkdir dir1 [dir2] ...");
        return -1;
    }

    for(int i = 1; i < argc; ++i)
    {
        filesys_dp_handle dp;
        char *path = malloc_and_cat_path(argv[0], argv[i], &dp);
        if(!path)
        {
            printf("%LInvalid directory name: %s", argv[i]);
            return -1;
        }

        compress_path(path);
        if(strlen(path) == 1)
        {
            printf("%LInvalid directory name: %s", argv[i]);
            return -1;
        }
        make_dir_chain(dp, path);

        free(path);
    }

    return 0;
}
