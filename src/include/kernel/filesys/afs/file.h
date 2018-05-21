#ifndef TINY_OS_FILESYS_AFS_FILE_H
#define TINY_OS_FILESYS_AFS_FILE_H

#include <kernel/filesys/afs/afs.h>
#include <kernel/filesys/afs/dp_phy.h>

#include <shared/intdef.h>
#include <shared/rbtree.h>

/* 一个打开的文件槽句柄 */
struct afs_file_desc;

void init_afs_file();

/* 在磁盘上创建一个大小为0的空文件，返回entry index */
uint32_t afs_create_empty_file(
            struct afs_dp_head *head,
            uint32_t type,
            enum afs_file_operation_status *rt);

/* 在磁盘上删除一个文件并释放其entry */
void afs_remove_file(struct afs_dp_head *head,
             uint32_t entry_idx,
             enum afs_file_operation_status *rt);

/*
    打开一个用于读的文件
    成功时返回desc指针
    若返回值为空，可检查rt指示的错误类型
*/
struct afs_file_desc *afs_open_file_for_reading(
                            struct afs_dp_head *head,
                            uint32_t entry_idx,
                            enum afs_file_operation_status *rt);

/* 打开一个可读写的文件 */
struct afs_file_desc *afs_open_file_for_writing(
                            struct afs_dp_head *head,
                            uint32_t entry_idx,
                            enum afs_file_operation_status *rt);

/* 尝试把一个以只读模式打开的文件转为以可读写模式打开，失败时返回false */
bool afs_convert_reading_to_writing(struct afs_dp_head *head,
                                    struct afs_file_desc *desc);

/* 关闭一个以只读方式打开的文件 */
void afs_close_file_for_reading(struct afs_dp_head *head,
                                struct afs_file_desc *file);

/* 关闭一个可读写的文件 */
void afs_close_file_for_writing(struct afs_dp_head *head,
                                struct afs_file_desc *file);

/*
    读某个文件中的二进制内容
    当读取范围超出文件大小时会失败
*/
bool afs_read_binary(struct afs_dp_head *head,
                     struct afs_file_desc *file,
                     uint32_t fpos, uint32_t size,
                     void *data,
                     enum afs_file_operation_status *rt);

/*
    将二进制内容写入到某个文件
    当文件大小不足时会失败
*/
bool afs_write_binary(struct afs_dp_head *head,
                      struct afs_file_desc *file,
                      uint32_t fpos, uint32_t bytes,
                      const void *data,
                      enum afs_file_operation_status *rt);

/* 扩充某个以可写方式打开的文件的大小，末尾扩充部分的内容为undefined */
bool afs_expand_file(struct afs_dp_head *head,
                     struct afs_file_desc *file,
                     uint32_t new_size,
                     enum afs_file_operation_status *rt);

/*
    查看某个文件是否处于被打开的状态
    一般由目录操作者调用，调用前最好把目录锁住
*/
bool afs_is_file_open(struct afs_dp_head *head, uint32_t entry);

struct afs_file_entry *afs_extract_file_entry(struct afs_file_desc *desc);

bool afs_is_file_wlocked(struct afs_file_desc *desc);

#endif /* TINY_OS_FILESYS_AFS_FILE_H */
