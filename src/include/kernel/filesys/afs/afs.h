#ifndef TINY_OS_FILESYS_AFS_AFS_H
#define TINY_OS_FILESYS_AFS_AFS_H

#include <kernel/filesys/afs/file_opr.h>

#include <shared/intdef.h>

struct afs_dp_head;

/*======================================================
目录文件格式描述：
    uint32_t count; // 目录项数量
    {
        char name[MAX + 1];   // 文件名
        uint32_t entry_index; // 文件entry位置
    } * count;
======================================================*/

void init_afs();

/* 单个文件最大长度 */
#define AFS_FILE_NAME_MAX_LENGTH 63

/*
    以下函数均为线程安全，遇到文件被其他线程使用时会打开失败
    具体调用结果应通过rt获得。无rt表明合法参数下函数执行不可能失败。
*/

struct afs_file_desc *afs_open_regular_file_for_reading(
                            struct afs_dp_head *head,
                            const char *path,
                            enum afs_file_operation_status *rt);

struct afs_file_desc * afs_open_regular_file_for_writing(
                            struct afs_dp_head *head,
                            const char *path,
                            enum afs_file_operation_status *rt);

void afs_close_regular_file(struct afs_dp_head *head,
                            struct afs_file_desc *file_desc);

struct afs_file_desc *afs_open_dir_file_for_reading(
                            struct afs_dp_head *head,
                            const char *path,
                            enum afs_file_operation_status *rt);

struct afs_file_desc *afs_open_dir_file_for_writing(
                            struct afs_dp_head *head,
                            const char *path,
                            enum afs_file_operation_status *rt);

void afs_close_dir_file(struct afs_dp_head *head,
                        struct afs_file_desc *file_desc);

#endif /* TINY_OS_FILESYS_AFS_AFS_H */
