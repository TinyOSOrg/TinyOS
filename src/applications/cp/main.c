#include <shared/path.h>
#include <shared/string.h>
#include <shared/sys.h>

#include <lib/mem.h>

int main(int argc, char *argv[])
{
    if(argc < 3)
    {
        printf("Usage: cp src dst\n");
        while(true);
    }

    const char *_src_name = argv[1];
    const char *_dst_name = argv[2];

    filesys_dp_handle cur_dp = get_dp_handle_from_path(argv[0], malloc, free);
    filesys_dp_handle src_dp, dst_dp;

    uint32_t src_buf_size = strlen(argv[0]) + strlen(_src_name) + 1;
    uint32_t dst_buf_size = strlen(argv[0]) + strlen(_dst_name) + 1;

    char *src_name = malloc(src_buf_size);
    char *dst_name = malloc(dst_buf_size);

    if(!cat_path_ex_s(cur_dp, skip_dp_in_abs_path(argv[0]),
                      _src_name, &src_dp, src_name, src_buf_size))
    {
        printf("Invalid src path: %s\n", _src_name);
        while(true);
    }

    if(!cat_path_ex_s(cur_dp, skip_dp_in_abs_path(argv[0]),
                      _dst_name, &dst_dp, dst_name, dst_buf_size))
    {
        printf("Invalid dst path: %s\n", _dst_name);
        while(true);
    }

    usr_file_handle sfp, dfp;
    if(open_file(src_dp, src_name, false, &sfp) != filesys_opr_success)
    {
        printf("Failed to open src file: %s\n", _src_name);
        while(true);
    }

    remove_file(dst_dp, dst_name); make_file(dst_dp, dst_name);
    if(open_file(dst_dp, dst_name, true, &dfp) != filesys_opr_success)
    {
        printf("Failed to open dst file: %s\n", _dst_name);
        while(true);
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

    printf("Done!\n");
    while(true);
    return 0;
}
