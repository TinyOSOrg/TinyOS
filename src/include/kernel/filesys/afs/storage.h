#ifndef TINY_OS_FILESYS_AFS_STORAGE_H
#define TINY_OS_FILESYS_AFS_STORAGE_H

#include <kernel/assert.h>
#include <kernel/filesys/afs/blkgroup.h>

#include <shared/intdef.h>

/* afs在磁盘上的存储结构组织
    一切都是文件，目录也是文件。
    文件类型并不记录在文件本身上，而是记录在其父文件到其的入口处。
    所有文件都以索引树结构进行存储，当一个文件扩充至原树不足以容纳时，就将树增高
*/

/*
    一个索引节点至多能容纳多少个孩子
    assertion: sizeof(struct afs_file_node_index) = block size
*/
#define AFS_FILE_NODE_MAX_CHILD_COUNT \
    ((int)((AFS_BLOCK_BYTE_SIZE - 12) / sizeof(afs_sector_index)))

/* 磁盘上的索引块结构 */
struct afs_file_index_node
{
    // 自己在索引树中的高度
    // 所有孩子的高度均为自己高度 - 1
    // 页节点（内容块）高度为0
    uint32_t height;

    // 有效的孩子数量
    uint32_t child_count;

    // 若height == 1，则last_child_content_size给出了最后一个孩子的有效大小
    uint32_t last_child_content_size;

    // children数组大小应保证：
    //    sizeof(struct afs_file_node_index) = AFS_BLOCK_BYTE_SIZE
    afs_sector_index children[AFS_FILE_NODE_MAX_CHILD_COUNT];
};

STATIC_ASSERT(sizeof(struct afs_file_index_node) == AFS_BLOCK_BYTE_SIZE,
             invalid_size_of_afs_file_index_node);

/*
    文件入口描述符
    目录文件中对其中每个文件的描述就长这样
*/
struct afs_file_entry
{
    // 文件本体有多少字节
    uint32_t byte_size;
    // 文件在磁盘中占用了多少字节
    uint32_t used_size;
    // 文件所处block的首个扇区号
    afs_sector_index sector;
};

/*=====================================================================
    文件flags含义
=====================================================================*/

#define AFS_FILE_SLOT_FLAG_IS_INDEX_MASK    0b00000000001; /* sector指向的是内容块还是索引块 */
#define AFS_FILE_SLOT_FLAG_TYPE_MASK        0b00000001110; /* 类型mask，取值见AFS_FILE_TYPE_XXX */
#define AFS_FILE_SLOT_FLAG_WRITE_LOCK_MASK  0b00000010000; /* 写入锁 */
#define AFS_FILE_SLOT_FLAG_READ_LOCK_MASK   0b11111100000; /* 读取锁 */

#define AFS_FILE_TYPE_REGULAR   (1 << 1) /* 文件类型取值：常规文件 */
#define AFS_FILE_TYPE_DIRECTORY (2 << 1) /* 文件类型取值：目录 */

#endif /* TINY_OS_FILESYS_AFS_STORAGE_H */
