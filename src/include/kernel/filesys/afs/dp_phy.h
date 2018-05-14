#ifndef TINY_OS_FILESYS_AFS_DP_PHY_H
#define TINY_OS_FILESYS_AFS_DP_PHY_H

#include <kernel/assert.h>
#include <kernel/diskdriver.h>
#include <kernel/process/semaphore.h>
#include <kernel/process/spinlock.h>

#include <shared/bool.h>
#include <shared/intdef.h>
#include <shared/rbtree.h>

/*======================================================
分区格式描述：
    afs_dp_head: 分区头部，一个扇区------------------
    entry数组: 一些扇区 <---------------|          |
    {                                            |
        afs_blkgrp_head: 块组头部，一个扇区         |
        AFS_BLOCK_BYTE_SIZE+                     |
    }+ <-----------------------------------------|
======================================================*/

/* 一个扇区多少字节 */
#define AFS_SECTOR_BYTE_SIZE 512

/* 一个block含多少扇区 */
#define AFS_BLOCK_SECTOR_COUNT 2

/*
    平均一个文件能够占用多少block
    用于格式化时估算需要的entry数量
*/
#define AFS_FILE_AVERAGE_BLOCK_COUNT 16

/* 一个block多少字节 */
#define AFS_BLOCK_BYTE_SIZE (AFS_BLOCK_SECTOR_COUNT * AFS_SECTOR_BYTE_SIZE)

/* 刚物理初始化的分区根目录入口 */
#define AFS_INIT_ROOT_DIR_ENTERY 0

/* 分区首个扇区的描述符 */
struct afs_dp_head
{
    // 分区扇区范围
    uint32_t dp_sec_beg;
    uint32_t dp_sec_cnt;

    // entry数组起始扇区号以及entry数组大小
    uint32_t entry_arr_sec;
    uint32_t entry_arr_len;

    // 还剩多少空闲entry
    uint32_t empty_entry_cnt;
    // 首个空闲entry在entry数组中的下标
    uint32_t fst_avl_entry_idx;

    // 首个block group的头部所在扇区号
    uint32_t fst_blkgrp_sec;

    // 还剩多少空闲block
    uint32_t empty_block_cnt;
    // 首个空闲block group的下标
    uint32_t fst_avl_blkgrp_idx;

    // 根目录entry_index
    uint32_t root_dir_entry;

    // 分区头部锁
    // 注意到这个结构也会被顺带写入到头部扇区中，但是需要专门初始化
    struct semaphore lock;

    // 分区活跃文件记录及其操作锁
    struct rb_tree opening_files; // entry_index -> afs_file_desc (file.c)
    spinlock opening_files_lock;
};

// 一个blkgrp至多包含多少block，由其head中的位图大小限制
#define AFS_BLKGRP_BLOCKS_MAX_COUNT (32 * ((AFS_SECTOR_BYTE_SIZE - 20) >> 2))

/*
    每个block group头部的描述符，固定占一个扇区
    afs将分区组织成多个block group，用于管理空闲的block
*/
struct afs_blkgrp_head
{
    // blkgrp范围，包括head自身在内
    uint32_t blkgrp_sec_beg;
    uint32_t blkgrp_sec_cnt;

    // 总共有多少block
    uint32_t all_blk_cnt;
    // 还剩多少block
    uint32_t empty_blk_cnt;

    // 下一个有空闲的blkgrp的blkgrp下标
    uint32_t next_avl_blkgrp;

    // block使用位图
    uint32_t blk_btmp[AFS_BLKGRP_BLOCKS_MAX_COUNT / 32];
};

// 一个完整的blkgrp总共占用多少扇区（包括头部）
#define AFS_COMPLETE_BLKGRP_SECTOR_COUNT \
    (1 + AFS_BLKGRP_BLOCKS_MAX_COUNT * AFS_BLOCK_SECTOR_COUNT)

STATIC_ASSERT(sizeof(struct afs_blkgrp_head) == AFS_SECTOR_BYTE_SIZE,
              invalid_size_of_afs_blkgrp_head);

/*
    文件entry
    每个afs分区都维护一个全局的entry数组作为文件描述符
    可以通过文件描述符下标来引用任意一个文件

    空闲的entry的首个uint32_t是下一个空闲entry的下标
    如果分区head中空闲entry数目为0,那么这个下标不具有任何含义
    blkgrp构成的自由链表同理
*/
struct afs_file_entry
{
    // 文件所在block的首个扇区号
    uint32_t sec_beg;

    // 文件字节数
    // 如果是目录文件，就为其所有子文件的字节数之和
    uint32_t byte_size;

    // 文件flags
    unsigned int index : 1;  // 索引标志
    unsigned int type  : 6;  // 类型
};

STATIC_ASSERT(sizeof(struct afs_file_entry) == 12,
              invalid_size_of_afs_file_entry);

#define AFS_FILE_TYPE_REGULAR   1
#define AFS_FILE_TYPE_DIRECTORY 2

/* 一个扇区至多包含多少个file entry */
#define AFS_SECTOR_MAX_FILE_ENTRY_COUNT \
    (AFS_SECTOR_BYTE_SIZE / sizeof(struct afs_file_entry))

/* 给定分区扇区范围[beg, beg + cnt)，在物理层上将其格式化，包括：
    1. 建立磁盘上的entry自由链表
    2. 建立磁盘上的各blkgrp的head
    3. 建立dp_head
*/
bool afs_phy_reformat_dp(uint32_t beg, uint32_t cnt);

#define afs_read_sector_raw(SEC, DATA)  disk_read(SEC, 1, DATA)
#define afs_read_block_raw(SEC, DATA)   disk_read(SEC, AFS_BLOCK_SECTOR_COUNT, DATA)
#define afs_write_sector_raw(SEC, DATA) disk_write(SEC, 1, DATA)
#define afs_write_block_raw(SEC, DATA)  disk_write(SEC, AFS_BLOCK_SECTOR_COUNT, DATA)

void afs_init_dp_head(uint32_t dp_beg, struct afs_dp_head *head);

void afs_restore_dp_head(struct afs_dp_head *head);

/*
    分配一个空闲块，返回其首个扇区LBA
    分配失败时返回0
*/
uint32_t afs_alloc_disk_block(struct afs_dp_head *head);

/*
    释放一个用afs_alloc_disk_block分配的块
    如果参数并非afs_alloc_disk_block分配，UB
*/
void afs_free_disk_block(struct afs_dp_head *head, uint32_t blk_sec);

/*
    分配一个新的文件入口并取得其在entry数组中的下标
    必须提供初始化数据
    若head中所有的entry已用完，返回false
*/
bool afs_alloc_file_entry(struct afs_dp_head *head,
                              const struct afs_file_entry *init_data,
                              uint32_t *entry_idx);

/*
    读取一个file_entry的数据
    若目标并非由afs_alloc_file_entry分配，UB
*/
void afs_read_file_entry(struct afs_dp_head *head, uint32_t idx,
                         struct afs_file_entry *data);

/*
    覆写一个已经存在的file_entry
    若目标并非由afs_alloc_file_entry分配，UB
*/
void afs_modify_file_entry(struct afs_dp_head *head, uint32_t idx,
                           const struct afs_file_entry *data);

/*
    释放一个已经存在的file_entry
    若目标并非由afs_alloc_file_entry分配，UB
*/
void afs_free_file_entry(struct afs_dp_head *head, uint32_t idx);

struct afs_file_entry *afs_access_file_entry_begin(
        struct afs_dp_head *head, uint32_t idx);

void afs_access_file_entry_end(struct afs_dp_head *head, uint32_t idx);

#endif /* TINY_OS_FILESYS_AFS_DP_PHY_H */
