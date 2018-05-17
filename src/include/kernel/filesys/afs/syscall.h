#ifndef TINY_OS_FILESYS_AFS_SYSCALL_H
#define TINY_OS_FILESYS_AFS_SYSCALL_H

#include <shared/intdef.h>

uint32_t syscall_afs_create_regular_impl(uint32_t path);

uint32_t syscall_afs_create_directory_impl(uint32_t path);

uint32_t syscall_afs_exists_impl(uint32_t path);

uint32_t syscall_afs_remove_regular_impl(uint32_t path);

uint32_t syscall_afs_remove_directory_impl(uint32_t path);

uint32_t syscall_afs_open_regular_impl(uint32_t path, uint32_t prt);

uint32_t syscall_afs_open_directory_impl(uint32_t path, uint32_t prt);

void syscall_afs_close_impl(uint32_t index);

#endif /* TINY_OS_FILESYS_AFS_SYSCALL_H */
