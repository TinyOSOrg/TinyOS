#ifndef TINY_OS_EXEC_ELF_EXEC_H
#define TINY_OS_EXEC_ELF_EXEC_H

#include <kernel/filesys/filesys.h>

/* exec_elf参数最大长度 */
#define EXEC_ELF_ARG_BUF_SIZE 256

/* 至多有多少参数 */
#define EXEC_ELF_ARG_MAX_COUNT 12

enum exec_elf_result
{
    exec_elf_success,       // 进程创建成功
    exec_elf_file_error,    // elf文件打开/读取出现错误
    exec_elf_invalid_elf,   // elf文件格式错误
};

/* 以磁盘上的一个elf文件为程序创建一个进程 */
enum exec_elf_result exec_elf(const char *proc_name,
                              filesys_dp_handle dp, const char *elf_path,
                              bool is_PL_0, uint32_t argc, const char **argv,
                              uint32_t *pid);

#endif /* TINY_OS_EXEC_ELF_EXEC_H */
