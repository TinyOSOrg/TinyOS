#include <shared/filesys.h>
#include <shared/string.h>

#include <lib/mem.h>
#include <lib/path.h>

char *malloc_and_cat_path(const char *input, const char *delta,
                          filesys_dp_handle *out_dp)
{
    filesys_dp_handle dp = get_dp_handle_from_path(input, malloc, free);
    if(dp >= DPT_UNIT_COUNT)
        return NULL;
    
    uint32_t size = strlen(input) + strlen(delta) + 1;
    char *ret = malloc(size);
    if(!ret)
        return NULL;

    if(!cat_path_ex_s(dp, skip_dp_in_abs_path(input),
                      delta, out_dp, ret, size))
    {
        free(ret);
        return NULL;
    }

    return ret;
}
