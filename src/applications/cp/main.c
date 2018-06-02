#include <shared/string.h>
#include <shared/sys.h>

#include <lib/input.h>
#include <lib/mem.h>
#include <lib/path.h>

int main(int argc, char *argv[])
{
    expl_new_line();

    if(argc < 3)
    {
        printf("Usage: cp src dst");
        return -1;
    }

    const char *_src_name = argv[1];
    const char *_dst_name = argv[2];

    filesys_dp_handle src_dp, dst_dp;
    char *src_name = malloc_and_cat_path(argv[0], _src_name, &src_dp);
    char *dst_name = malloc_and_cat_path(argv[0], _dst_name, &dst_dp);
    
    if(!src_name)
    {
        printf("Invalid src path: %s", _src_name);
        return -1;
    }

    if(!dst_name)
    {
        printf("Invalid dst path: %s", _dst_name);
        return -1;
    }

    usr_file_handle sfp, dfp;
    if(open_file(src_dp, src_name, false, &sfp) != filesys_opr_success)
    {
        printf("Failed to open src file: %s", _src_name);
        return -1;
    }

    remove_file(dst_dp, dst_name); make_file(dst_dp, dst_name);
    if(open_file(dst_dp, dst_name, true, &dfp) != filesys_opr_success)
    {
        printf("Failed to open dst file: %s", _dst_name);
        return -1;
    }

    uint32_t file_size = get_file_size(sfp);
    uint8_t buf[128]; uint32_t fpos = 0;

    while(fpos < file_size)
    {
        uint32_t delta = file_size - fpos;
        if(delta > 128)
            delta = 128;
        read_file(sfp, fpos, delta, buf);
        write_file(dfp, fpos, delta, buf);
        fpos += delta;
    }

    close_file(sfp);
    close_file(dfp);

    printf("Done");

    return 0;
}
