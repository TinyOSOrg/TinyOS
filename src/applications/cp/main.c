#include <shared/path.h>
#include <shared/string.h>
#include <shared/sys.h>

#include <lib/mem.h>

int main(int argc, char *argv[])
{
    if(argc < 4)
    {
        printf("Usage: cp src dst\n");
        while(true);
    }

    const char *src_name = argv[2];
    const char *dst_name = argv[3];

    usr_file_handle sfp, dfp;
    if(open_file(0, src_name, false, &sfp) != filesys_opr_success)
    {
        printf("Failed to open src file: %s\n", src_name);
        while(true);
    }

    remove_file(0, dst_name);
    make_file(0, dst_name);
    if(open_file(0, dst_name, true, &dfp) != filesys_opr_success)
    {
        printf("Failed to open dst file: %s\n", dst_name);
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
