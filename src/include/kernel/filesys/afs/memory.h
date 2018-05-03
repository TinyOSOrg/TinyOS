#ifndef TINY_OS_FILESYS_AFS_MEMORY_H
#define TINY_OS_FILESYS_AFS_MEMORY_H

#include <kernel/filesys/afs/storage.h>

/* 一个afs分区的描述符 */
struct afs_handle
{
    // 该分区在分区表中的索引
    uint32_t dp_index;

    // 分区LBA扇区范围：[begin, end)
    afs_sector_index sector_begin, sector_end;

    // 根目录入口文件扇区号
    afs_sector_index root_dir_sec;

    // 首个可用block group头部
    struct afs_block_group_head *fst_available_blkgrp;
};

#endif /* TINY_OS_FILESYS_AFS_MEMORY_H */
