#ifndef TINY_OS_FILESYS_AFS_BLOCK_GROUP_H
#define TINY_OS_FILESYS_AFS_BLOCK_GROUP_H

#include <kernel/assert.h>

#include <shared/intdef.h>

/*
    afs如何组织磁盘上的空闲块

    分区被划分为多个大小相等的block group
    每个block group头部用链表记录该group内的空闲块
    有空闲的block group用自由链表组织在磁盘上
    自由链表头部的block group常驻内存

    设
*/

/* afs扇区LBA */
typedef uint32_t afs_sector_index;

/* block group编号 */
typedef uint32_t afs_blkgroup_index;

/* 每个block有多少个扇区 */
#define AFS_BLOCK_SECTOR_COUNT 2

/* 每个block多少字节 */
#define AFS_BLOCK_BYTE_SIZE (512 * AFS_BLOCK_SECTOR_COUNT)

/* block group头部中的链表节点 */
struct afs_block_group_inner_freelist_node
{
    // last和next都是afs_block_group_head::blocks的下标
    uint16_t last;
    uint16_t next;
};

/*
    一个block group至多有多少个空block
    加上block group head会占用一个block，一个block group占磁盘空间至多为
        AFS_BLOCK_GROUP_MAX_EMPTY_BLOCKS_COUNT + 1
    当然实际上由于分区大小不一定是这个值的整数倍，每个分区末尾的block一般会小一些。
 */
#define AFS_BLOCK_GROUP_MAX_EMPTY_BLOCKS_COUNT \
    ((int)((AFS_BLOCK_BYTE_SIZE - 16) / sizeof(struct afs_block_group_inner_freelist_node)))

/* 磁盘上的block group结构 */
struct afs_block_group_head
{
    // 总block数量
    uint32_t total_blocks;
    // 空闲block数量
    uint32_t empty_blocks;
    // 后继非空block group编号
    afs_blkgroup_index next_block_group;
    // 空闲块自由链表头部节点在empty_blocks中的下标
    uint32_t empty_block_freelist_entry;
    // 空闲块自由链表空间
    struct afs_block_group_inner_freelist_node
        blocks[AFS_BLOCK_GROUP_MAX_EMPTY_BLOCKS_COUNT];
};

STATIC_ASSERT(sizeof(struct afs_block_group_head) == AFS_BLOCK_BYTE_SIZE,
              invalid_size_of_afs_block_group_head);

#endif /* TINY_OS_FILESYS_AFS_BLOCK_GROUP_H */
