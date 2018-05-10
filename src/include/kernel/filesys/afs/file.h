#ifndef TINY_OS_FILESYS_AFS_FILE_H
#define TINY_OS_FILESYS_AFS_FILE_H

#include <kernel/filesys/afs/dp_phy.h>

#include <shared/intdef.h>
#include <shared/rbtree.h>

#define AFS_FILE_PATH_MAX_LENGTH 255
#define AFS_FILE_PATH_BUF_SIZE (AFS_FILE_PATH_MAX_LENGTH + 1)

enum afs_file_operation_status
{
    afs_file_opr_success,           // 操作正常完成
    afs_file_opr_not_found,         // 要打开的文件不存在
    afs_file_opr_writing_lock,      // 试图打开一个有写入锁的文件
    afs_file_opr_reading_lock,      // 试图以读写模式打开一个有读取锁的文件
    afs_file_opr_not_opening,       // 要关闭的文件并不在已打开文件列表中
    afs_file_opr_limit_exceeded,    // 文件读写范围超出文件大小
    afs_file_opr_no_empty_entry,    // 创建文件失败：没有空闲的entry
    afs_file_opr_no_empty_block,    // 创建/扩充文件失败：没有空闲的block
};

/* 一个打开的文件槽句柄 */
struct afs_file_desc;

void init_afs_file(void);

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

/* 关闭一个以只读方式打开的文件 */
void afs_close_file_for_reading(struct afs_dp_head *head,
        struct afs_file_desc *file, enum afs_file_operation_status *rt);

/* 关闭一个可读写的文件 */
void afs_close_file_for_writing(struct afs_dp_head *head,
        struct afs_file_desc *file, enum afs_file_operation_status *rt);

/* 读某个文件中的二进制内容，当读取范围超出文件大小时会失败*/
bool afs_read_binary(struct afs_dp_head *head,
        struct afs_file_desc *file, uint32_t fpos,
        uint32_t size, void *data);

/*
    将二进制内容写入到某个文件
    当文件大小不足时会失败
*/
bool afs_write_binary(struct afs_dp_head *head,
        struct afs_file_desc *file, uint32_t fpos,
        uint32_t size, const void *data);

/* 扩充某个以可写方式打开的文件的大小，末尾扩充部分自动填0 */
bool afs_expand_file(struct afs_dp_head *head,
        struct afs_file_desc *file, uint32_t new_size);

#endif /* TINY_OS_FILESYS_AFS_FILE_H */
