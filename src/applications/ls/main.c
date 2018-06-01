#include <shared/path.h>
#include <shared/sys.h>

#include <lib/mem.h>

int main(int argc, char *argv[])
{
    expl_new_line();

    // 解析自己现在所在分区和路径

    filesys_dp_handle dp = get_dp_handle_from_path(
        argv[0], malloc, free);
    if(dp >= DPT_UNIT_COUNT)
    {
        printf("Invalid dp name");
        return -1;
    }
    const char *path = skip_dp_in_abs_path(argv[0]);

    uint32_t file_cnt;
    if(get_child_file_count(dp, path, &file_cnt) != filesys_opr_success)
    {
        printf("Failed to open directory: %s", argv[0]);
        return -1;
    }

    printf("File count: %u", file_cnt);

    for(uint32_t i = 0; i < file_cnt; ++i)
    {
        struct syscall_filesys_file_info info;
        if(get_child_file_info(dp, path, i, &info) != filesys_opr_success)
        {
            printf("\nFailed to fetch file into: %s, %u", argv[0], i);
            return -1;
        }
        printf("\n%s  %s", (info.is_dir ? "d" : "r"), info.name);
    }

    return 0;
}
