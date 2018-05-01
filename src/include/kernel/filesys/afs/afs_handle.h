#ifndef TINY_OS_FILESYS_AFS_AFS_HANDLE_H
#define TINY_OS_FILESYS_AFS_AFS_HANDLE_H

#include <shared/intdef.h>

/* afs扇区LBA */
typedef uint32_t afs_sector_index;

/* 每个block有多少个扇区 */
#define AFS_BLOCK_SECTOR_COUNT 2

/* 每个block多少字节 */
#define AFS_BLOCK_BYTE_SIZE (512 * AFS_BLOCK_SECTOR_COUNT)

/* block头 */
struct afs_block_head
{
    afs_sector_index last;
    afs_sector_index next;
};

/* 块占位类型 */
struct afs_file_node_data
{
    uint8_t data[AFS_BLOCK_BYTE_SIZE];
};

/*
    一个索引节点至多能容纳多少个孩子
    assertion: sizeof(struct afs_file_node_index) = block size
*/
#define AFS_FILE_NODE_MAX_CHILD_COUNT \
    ((int)((AFS_BLOCK_BYTE_SIZE - 12) / sizeof(afs_sector_index)))

/* 索引块结构 */
struct afs_file_node_index
{
    // 自己在索引树中的高度
    // 所有孩子的高度均为自己高度 - 1
    // 页节点（内容块）高度为0
    uint32_t height;

    // 有效的孩子数量
    uint32_t child_count;

    // 若height == 1，则last_child_content_size给出了最后一个孩子的有效大小
    uint32_t last_child_content_size;

    afs_sector_index children[AFS_FILE_NODE_MAX_CHILD_COUNT];
};

/* 一个afs分区的描述符 */
struct afs_handle
{
    // 该分区在分区表中的索引
    uint32_t dp_index;

    // 分区LBA扇区范围：[begin, end)
    afs_sector_index sector_begin, sector_end;

    // 根目录入口文件扇区号
    afs_sector_index root_dir_sec;

    // 根目录文件块
    struct afs_file_node_data root_dir_root;
};

#endif /* TINY_OS_FILESYS_AFS_AFS_HANDLE_H */
