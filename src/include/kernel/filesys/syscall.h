#ifndef TINY_OS_FILESYS_SYSCALL_H
#define TINY_OS_FILESYS_SYSCALL_H

#include <shared/filesys.h>
#include <shared/stdbool.h>
#include <shared/stdint.h>

enum filesys_opr_result syscall_filesys_open_impl(
        struct syscall_filesys_open_params *params);

enum filesys_opr_result syscall_filesys_close_impl(
        usr_file_handle file);

enum filesys_opr_result syscall_filesys_mkfile_impl(
        filesys_dp_handle dp, const char *path);

enum filesys_opr_result syscall_filesys_rmfile_impl(
        filesys_dp_handle dp, const char *path);

enum filesys_opr_result syscall_filesys_mkdir_impl(
        filesys_dp_handle dp, const char *path);

enum filesys_opr_result syscall_filesys_rmdir_impl(
        filesys_dp_handle dp, const char *path);

uint32_t syscall_filesys_get_file_size_impl(
        usr_file_handle file);

enum filesys_opr_result syscall_filesys_write_impl(
        struct syscall_filesys_write_params *params);

enum filesys_opr_result syscall_filesys_read_impl(
        struct syscall_filesys_read_params *params);

enum filesys_opr_result syscall_filesys_get_file_count(
            filesys_dp_handle dp, const char *path,
            uint32_t *rt);

enum filesys_opr_result syscall_filesys_get_child_info(
            struct syscall_filesys_get_child_info_params *params);

#endif /* TINY_OS_FILESYS_SYSCALL_H */
