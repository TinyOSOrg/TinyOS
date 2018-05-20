#ifndef TINY_OS_SHARED_SYSCALL_FILESYS_H
#define TINY_OS_SHARED_SYSCALL_FILESYS_H

#include <shared/atrc.h>
#include <shared/filesys/filesys.h>

/*
    文件系统相关的系统调用的参数结构体
*/

typedef atrc_elem_handle usr_file_handle;

struct syscall_filesys_open_params
{
    bool writing;
    filesys_dp_handle dp;
    const char *path;
    usr_file_handle *result;
};

struct syscall_filesys_read_params
{
    usr_file_handle file;
    uint32_t fpos, byte_size;
    void *data_dst;
};

struct syscall_filesys_write_params
{
    usr_file_handle file;
    uint32_t fpos, byte_size;
    const void *data_src;
};

#endif /* TINY_OS_SHARED_SYSCALL_FILESYS_H */
