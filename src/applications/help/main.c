#include <shared/string.h>
#include <shared/sys.h>

#include <lib/fstm.h>
#include <lib/input.h>
#include <lib/mem.h>

#define DOC_DP 0
#define DOC_PATH_PREFIX ("/docs/")

int main(int argc, char *argv[])
{
    if(argc > 2)
    {
        printf("%LUsage: help\n");
        printf("         help [command]");
        return -1;
    }

    const char *arg      = (argc == 2) ? argv[1] : "help";
    size_t path_buf_size = strlen(DOC_PATH_PREFIX) + strlen(arg) + 1;
    char *path           = malloc(path_buf_size);
    if(!path)
    {
        printf("%LMemory allocation failed...");
        return -1;
    }
    strcpy(path, DOC_PATH_PREFIX);
    strcat(path, arg);
    
    ifstm_t *f = ifstm_init(DOC_DP, path, FSTM_BUF_SIZE_DEFAULT);
    if(!f)
    {
        printf("%LNo documentation for command '%s'", arg);
        return -1;
    }

    expl_new_line();
    char ch;
    while((ch = ifstm_next(f)) != FSTM_EOF)
        put_char(ch);
    
    ifstm_free(f);
    return 0;
}
