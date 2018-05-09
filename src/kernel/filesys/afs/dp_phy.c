#include <kernel/asm.h>
#include <kernel/assert.h>
#include <kernel/diskdriver.h>

#include <kernel/filesys/afs/blk_mem_buf.h>
#include <kernel/filesys/afs/dp_phy.h>
#include <kernel/filesys/afs/sector_cache.h>

#include <shared/string.h>

static inline size_t ceil_int_div(size_t a, size_t b)
{
    return (a / b) + (a % b ? 1 : 0);
}

/* 将blkgrp下标转换为其头部描述符所在的扇区号 */
static inline uint32_t blkgrp_idx_to_head_sec(struct afs_dp_head *dph, uint32_t idx)
{
    return dph->fst_blkgrp_sec + idx * AFS_COMPLETE_BLKGRP_SECTOR_COUNT;
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

        afs_write_sector_raw(beg + 1 + es_i, entry_buf);
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
        
        afs_write_sector_raw(blkgrp_sec_beg, blkgrp_head);
        
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
    dp_head->fst_blkgrp_sec = beg + 1 + entry_sec_cnt;

    afs_write_sector_raw(beg, dp_head);

    afs_free_block_buffer(dp_head);

    return true;
}

uint32_t afs_alloc_disk_block(struct afs_dp_head *head)
{
    if(!head->empty_block_cnt)
        return 0;
    
    // 取得首个有空闲的blkgrp的头部描述符
    uint32_t fst_avl_blkgrp_head_sec =
        blkgrp_idx_to_head_sec(head, head->fst_avl_blkgrp_idx);
    struct afs_blkgrp_head *blkgrp_head = (struct afs_blkgrp_head *)
        afs_write_to_sector_begin(fst_avl_blkgrp_head_sec);
    
    ASSERT_S(blkgrp_head->empty_blk_cnt != 0);

    // 在block group头部位图中搜索一个可用block
    uint32_t g_idx = 0, l_idx = 0;
    while(true)
    {
        if(blkgrp_head->blk_btmp[g_idx])
        {
            l_idx = _find_lowest_nonzero_bit(blkgrp_head->blk_btmp[g_idx]);
            blkgrp_head->blk_btmp[g_idx] &= ~(1 << l_idx);
            break;
        }
        ++g_idx;
    }
    
    head->empty_block_cnt--;
    blkgrp_head->empty_blk_cnt--;

    // 如果该blkgrp已经用尽，更新head中的首个可用blkgrp编号
    if(!blkgrp_head->empty_blk_cnt)
        head->fst_avl_blkgrp_idx = blkgrp_head->next_avl_blkgrp;

    afs_write_to_sector_end(fst_avl_blkgrp_head_sec);

    return fst_avl_blkgrp_head_sec + 1 +
        AFS_BLOCK_SECTOR_COUNT * (32 * g_idx + l_idx);
}

void afs_free_disk_block(struct afs_dp_head *head, uint32_t blk_sec)
{
    // 计算blk_sec处于head中的哪个blkgrp中
    uint32_t blkgrp_idx = (blk_sec - head->fst_blkgrp_sec)
                        / AFS_COMPLETE_BLKGRP_SECTOR_COUNT;
    
    // 取得对应blkgrp的头部描述符
    uint32_t blkgrp_head_sec = blkgrp_idx_to_head_sec(head, blkgrp_idx);
    struct afs_blkgrp_head *blkgrp_head = (struct afs_blkgrp_head *)
        afs_write_to_sector_begin(blkgrp_head_sec);
    
    // 把blk_sec对应的位图位置1
    uint32_t l_offset = (blk_sec - (blkgrp_head_sec + 1))
                      / AFS_BLOCK_SECTOR_COUNT;
    ASSERT_S(l_offset < blkgrp_head->all_blk_cnt);
    blkgrp_head->blk_btmp[l_offset >> 5] |= (1 << (l_offset & 0b11111));

    head->empty_block_cnt++;
    blkgrp_head->empty_blk_cnt++;

    // 检查是否需要把这个blkgrp重新链入空闲blkgrp链表中
    if(blkgrp_head->empty_blk_cnt == 1)
    {
        blkgrp_head->next_avl_blkgrp = head->fst_avl_blkgrp_idx;
        head->fst_avl_blkgrp_idx = blkgrp_idx;
    }

    afs_write_to_sector_end(blkgrp_head_sec);
}

#define ENTRY_INDEX_TO_GLOBAL_LOCAL_INDEX(IDX, GBL, LCL) \
    do { \
        (GBL) = (IDX) / AFS_SECTOR_MAX_FILE_ENTRY_COUNT; \
        (LCL) = (IDX) % AFS_SECTOR_MAX_FILE_ENTRY_COUNT; \
    } while(0)

uint32_t afs_alloc_file_entry(struct afs_dp_head *head,
                              const struct afs_file_entry *init_data)
{
    ASSERT_S(head && head->empty_entry_cnt != 0 && init_data);

    // 计算首个空闲entry所处的扇区号及扇区内偏移
    uint32_t ret = head->fst_avl_entry_idx;
    uint32_t gbl, lcl;
    ENTRY_INDEX_TO_GLOBAL_LOCAL_INDEX(ret, gbl, lcl);
    uint32_t sec = head->dp_sec_beg + 1 + gbl;

    struct afs_file_entry *entrys = (struct afs_file_entry*)
        afs_write_to_sector_begin(sec);

    head->empty_entry_cnt--;
    head->fst_avl_entry_idx = *(uint32_t*)&entrys[lcl];

    memcpy((char*)&entrys[lcl], (const char *)init_data,
           sizeof(struct afs_file_entry));

    afs_write_to_sector_end(sec);

    return ret;
}

void afs_read_file_entry(struct afs_dp_head *head, uint32_t idx,
                         struct afs_file_entry *data)
{
    uint32_t gbl, lcl, sec;
    ENTRY_INDEX_TO_GLOBAL_LOCAL_INDEX(idx, gbl, lcl);
    sec = head->dp_sec_beg + 1 + gbl;

    afs_read_from_sector(sec, sizeof(struct afs_file_entry) * lcl,
                         sizeof(struct afs_file_entry), data);
}

void afs_modify_file_entry(struct afs_dp_head *head, uint32_t idx,
                           const struct afs_file_entry *data)
{
    uint32_t gbl, lcl, sec;
    ENTRY_INDEX_TO_GLOBAL_LOCAL_INDEX(idx, gbl, lcl);
    sec = head->dp_sec_beg + 1 + gbl;

    afs_write_to_sector(sec, sizeof(struct afs_file_entry) * lcl,
                        sizeof(struct afs_file_entry), data);
}

void afs_free_file_entry(struct afs_dp_head *head, uint32_t idx)
{
    uint32_t gbl, lcl, sec;
    ENTRY_INDEX_TO_GLOBAL_LOCAL_INDEX(idx, gbl, lcl);
    sec = head->dp_sec_beg + 1 + gbl;

    struct afs_file_entry *entrys = (struct afs_file_entry*)
        afs_write_to_sector_begin(sec);
    
    *(uint32_t*)&entrys[lcl] = head->fst_avl_entry_idx;
    head->fst_avl_entry_idx = idx;
    head->empty_entry_cnt++;

    afs_write_to_sector_end(sec);
}
