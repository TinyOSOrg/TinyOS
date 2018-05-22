#ifndef TINY_OS_FILESYS_AFS_AFS_H
#define TINY_OS_FILESYS_AFS_AFS_H

#include <kernel/filesys/afs/file_opr.h>
#include <kernel/filesys/afs/file_type.h>

#include <shared/bool.h>
#include <shared/intdef.h>

struct afs_dp_head;
struct afs_file_desc;

/*======================================================
目录文件格式描述：
    uint32_t count; // 目录项数量
    uint32_t zone;  // 目录项空间大小
    {
        char name[MAX + 1];   // 文件名
        uint32_t entry_index; // 文件entry位置
    } * count;
======================================================*/

void init_afs();

void destroy_afs();

/* 单个文件名最大长度 */
#define AFS_FILE_NAME_MAX_LENGTH 63

/*
    以下函数均为线程安全，遇到互斥时会操作失败
    具体调用结果应通过rt获得，无rt形参表示在参数合法时函数执行不可能失败。
*/

bool afs_reformat_dp(uint32_t beg, uint32_t cnt);

struct afs_dp_head *afs_init_dp_handler(uint32_t beg);

void afs_restore_dp_handler(uint32_t handler);

void afs_release_dp_handler(uint32_t handler);

struct afs_file_desc *afs_open_regular_file_for_reading_by_path(
                            struct afs_dp_head *head,
                            const char *path,
                            enum afs_file_operation_status *rt);

struct afs_file_desc * afs_open_regular_file_for_writing_by_path(
                            struct afs_dp_head *head,
                            const char *path,
                            enum afs_file_operation_status *rt);

struct afs_file_desc *afs_open_dir_file_for_reading_by_path(
                            struct afs_dp_head *head,
                            const char *path,
                            enum afs_file_operation_status *rt);

struct afs_file_desc *afs_open_dir_file_for_writing_by_path(
                            struct afs_dp_head *head,
                            const char *path,
                            enum afs_file_operation_status *rt);

void afs_close_file(struct afs_dp_head *head,
                    struct afs_file_desc *file_desc);

uint32_t afs_create_dir_file_raw(struct afs_dp_head *head,
                                 uint32_t parent_dir, bool root,
                                 enum afs_file_operation_status *rt);

void afs_create_dir_file_by_path(struct afs_dp_head *head,
                                 const char *path,
                                 enum afs_file_operation_status *rt);

void afs_create_regular_file_by_path(struct afs_dp_head *head,
                             const char *path,
                             enum afs_file_operation_status *rt);

void afs_remove_file_by_path(struct afs_dp_head *head,
                             const char *path,
                             uint32_t type,
                             enum afs_file_operation_status *rt);

uint32_t afs_get_file_byte_size(struct afs_file_desc *file);

void afs_expand_regular_file(struct afs_dp_head *head,
                             struct afs_file_desc *file,
                             uint32_t new_size,
                             enum afs_file_operation_status *rt);

#endif /* TINY_OS_FILESYS_AFS_AFS_H */
