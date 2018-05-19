#ifndef TINY_OS_FILESYS_SYSCALL_H
#define TINY_OS_FILESYS_SYSCALL_H

#include <shared/bool.h>
#include <shared/filesys/filesys.h>
#include <shared/intdef.h>
#include <shared/syscall/filesys.h>

file_handle syscall_filesys_open_impl(bool writing, const char *path,
                                      enum filesys_opr_result *rt);

enum filesys_opr_result syscall_filesys_close_impl(filesys_dp_handle dp,
                                                   file_handle file);

enum filesys_opr_result syscall_filesys_mkfile_impl(const char *path);

enum filesys_opr_result syscall_filesys_rmfile_impl(const char *path);

enum filesys_opr_result syscall_filesys_mkdir_impl(const char *path);

enum filesys_opr_result syscall_filesys_rmdir_impl(const char *path);

uint32_t syscall_filesys_get_file_size_impl(filesys_dp_handle dp,
                                            file_handle file);

enum filesys_opr_result syscall_filesys_write_impl(
        struct syscall_filesys_write_params *params);

enum filesys_opr_result syscall_filesys_read_impl(
        struct syscall_filesys_read_params *params);

#endif /* TINY_OS_FILESYS_SYSCALL_H */
