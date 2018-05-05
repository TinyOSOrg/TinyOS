#include <kernel/assert.h>
#include <kernel/diskdriver.h>
#include <kernel/filesys/afs/blk_mem_buf.h>
#include <kernel/filesys/afs/dp_phy.h>

#include <shared/string.h>

static inline size_t ceil_int_div(size_t a, size_t b)
{
    return (a / b) + (a % b ? 1 : 0);
}

bool afs_phy_reformat_dp(uint32_t beg, uint32_t cnt)
{
    //********************* 各种数量分配 *********************

    ASSERT_S(cnt > 0);

    // 设一个文件平均占用A个block，理论上我们能够精确地解出一个最佳entry数量来
    // 但是一个block的空间就够好几十个entry
    // 所以我们直接忽略元数据占用的空间，强行算一个entry数量出来
    uint32_t entry_cnt = (cnt - 1) /
                         AFS_BLOCK_SECTOR_COUNT /
                         AFS_FILE_AVERAGE_BLOCK_COUNT;
    if(1 + entry_cnt >= cnt || entry_cnt < 1)
        return false;
    
    // entry数组占用的扇区数
    uint32_t entry_sec_cnt = ceil_int_div(
        entry_cnt, AFS_SECTOR_MAX_FILE_ENTRY_COUNT);
    
    // 所有block group的总扇区数
    uint32_t total_blkgrp_sec_cnt = cnt - 1 - entry_cnt;
    if(total_blkgrp_sec_cnt < 1)
        return false;
    
    // block group数量
    uint32_t blkgrp_cnt = ceil_int_div(
        total_blkgrp_sec_cnt, AFS_COMPLETE_BLKGRP_SECTOR_COUNT);
    // 最后一个block group（很可能是不完整的）占用了多少个扇区
    uint32_t last_blkgrp_sec_cnt =
        blkgrp_cnt % AFS_COMPLETE_BLKGRP_SECTOR_COUNT;
    // 最后一个block group有多少block
    uint32_t last_blkgrp_blk_cnt = (last_blkgrp_sec_cnt - 1) /
        AFS_BLOCK_SECTOR_COUNT;
    // 如果最后一个blkgrp根本没block可用,就把那点空间扔掉
    if(!last_blkgrp_blk_cnt)
    {
        --blkgrp_cnt;
        last_blkgrp_sec_cnt = AFS_COMPLETE_BLKGRP_SECTOR_COUNT;
        last_blkgrp_blk_cnt = AFS_BLKGRP_BLOCKS_MAX_COUNT;
    }

    // 一个block group都没有的分区，连根目录都没法建立
    if(!blkgrp_cnt)
        return false;

    //********************* 写入entry结构 *********************

    struct afs_file_entry *entry_buf =
        (struct afs_file_entry *)afs_alloc_block_buffer();
    
    uint32_t entry_idx = 0;     // 总entry下标

    // 以扇区为单位写入entry数组
    // 所有entry都是空闲的，形成链表：
    // head -> 0 -> 1 -> 2 -> ... -> (entry_cnt - 1) -> entry_cnt
    // 最后的entry_cnt实际上是非法的，不过没关系，因为当empty_entry_cnt数量
    // 为0的时候指向的下标是不会被使用的
    for(uint32_t es_i = 0;es_i != entry_sec_cnt; ++es_i)
    {
        for(uint32_t ei = 0;
            ei < AFS_SECTOR_MAX_FILE_ENTRY_COUNT &&
            entry_idx < entry_cnt;
            ++ei, ++entry_idx)
        {
            *(uint32_t*)&entry_buf[ei] = entry_idx + 1;
        }

        struct disk_rw_task task =
        {
            .type           = DISK_RW_TASK_TYPE_WRITE,
            .sector_base    = beg + 1 + es_i,
            .sector_cnt     = 1,
            .addr.write_src = entry_buf
        };
        disk_rw_raw(&task);
    }

    afs_free_block_buffer(entry_buf);

    //********************* 写入blkgrps *********************

    struct afs_blkgrp_head *blkgrp_head =
        (struct afs_blkgrp_head *)afs_alloc_block_buffer();

    // 当前正在写入的blkgrp的起始扇区号
    uint32_t blkgrp_sec_beg = beg + 1 + entry_sec_cnt;

    // 总空闲block数量
    uint32_t total_empty_blk_cnt = 0;

    for(uint32_t i = 0;i != blkgrp_cnt; ++i)
    {
        // 本block group有多少block
        uint32_t blk_cnt = (i + 1 == blkgrp_cnt) ?
            AFS_BLKGRP_BLOCKS_MAX_COUNT :
            last_blkgrp_blk_cnt;
        total_empty_blk_cnt += blk_cnt;
        
        blkgrp_head->blkgrp_sec_beg = blkgrp_sec_beg;
        blkgrp_head->blkgrp_sec_cnt = 1 + blk_cnt * AFS_BLOCK_SECTOR_COUNT;

        blkgrp_head->all_blk_cnt   = blk_cnt;
        blkgrp_head->empty_blk_cnt = blk_cnt;

        blkgrp_head->next_avl_blkgrp = i + 1;

        memset((char*)blkgrp_head->blk_btmp, 0xff,
               AFS_BLKGRP_BLOCKS_MAX_COUNT / 8);
        
        struct disk_rw_task task =
        {
            .type           = DISK_RW_TASK_TYPE_WRITE,
            .sector_base    = blkgrp_sec_beg,
            .sector_cnt     = 1,
            .addr.write_src = blkgrp_head
        };
        disk_rw_raw(&task);
        
        blkgrp_sec_beg += AFS_COMPLETE_BLKGRP_SECTOR_COUNT;
    }

    afs_free_block_buffer(blkgrp_head);

    //********************* 写入dp_head *********************

    struct afs_dp_head *dp_head =
        (struct afs_dp_head *)afs_alloc_block_buffer();
    
    dp_head->dp_sec_beg = beg;
    dp_head->dp_sec_cnt = cnt;

    dp_head->entry_arr_sec = beg + 1;
    dp_head->entry_arr_len = entry_cnt;

    dp_head->empty_entry_cnt = entry_cnt;
    dp_head->fst_avl_entry_idx = 0;

    dp_head->fst_avl_blkgrp_idx = 0;
    dp_head->empty_block_cnt = total_empty_blk_cnt;

    struct disk_rw_task dp_head_task =
    {
        .type           = DISK_RW_TASK_TYPE_WRITE,
        .sector_base    = beg,
        .sector_cnt     = 1,
        .addr.write_src = dp_head
    };
    disk_rw_raw(&dp_head_task);

    afs_free_block_buffer(dp_head);

    return true;
}

void afs_read_dp_head(uint32_t sec, struct afs_dp_head *output)
{
    ASSERT_S(output);

    struct disk_rw_task task =
    {
        .type          = DISK_RW_TASK_TYPE_READ,
        .sector_base   = sec,
        .sector_cnt    = 1,
        .addr.read_dst = output
    };
    disk_rw_raw(&task);
}

void afs_write_dp_head(uint32_t sec, const struct afs_dp_head *input)
{
    ASSERT_S(input);

    struct disk_rw_task task =
    {
        .type           = DISK_RW_TASK_TYPE_WRITE,
        .sector_base    = sec,
        .sector_cnt     = 1,
        .addr.write_src = input
    };
    disk_rw_raw(&task);
}
