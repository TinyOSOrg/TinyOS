#include <kernel/diskdriver.h>
#include <kernel/filesys/afs/afs.h>
#include <kernel/filesys/afs/memory.h>
#include <kernel/filesys/afs/storage.h>
#include <kernel/interrupt.h>
#include <kernel/memory.h>

#include <shared/freelist.h>
#include <shared/string.h>

/* 空闲的afs handle自由链表 */
static freelist_handle empty_afs_handle_freelist;

/* block缓存自由链表 */
static freelist_handle empty_block_buffer_freelist;

/* 申请一个未初始化的afs handle */
static struct afs_handle *alloc_afs_handle(void)
{
    if(is_freelist_empty(&empty_afs_handle_freelist))
    {
        struct afs_handle *new_handles = (struct afs_handle*)
            alloc_ker_page(true);
        size_t end = 4096 / sizeof(struct afs_handle);
        for(size_t i = 0;i != end; ++i)
            add_freelist(&empty_afs_handle_freelist, &new_handles[i]);
    }
    return fetch_freelist(&empty_afs_handle_freelist);
}

/* 释放一块不再使用的afs handle空间 */
static inline void release_afs_handle_zone(struct afs_handle *handle)
{
    add_freelist(&empty_afs_handle_freelist, handle);
}

/* 申请一块空闲的block缓存 */
static void *alloc_block_buffer(void)
{
    if(is_freelist_empty(&empty_block_buffer_freelist))
    {
        char *data = (char*)alloc_ker_page(true);
        size_t end = 4096 / AFS_BLOCK_BYTE_SIZE;
        for(size_t i = 0;i != end; ++i)
            add_freelist(&empty_block_buffer_freelist,
                         data + AFS_BLOCK_BYTE_SIZE * i);
        ASSERT_S(end == 4);
    }
    return fetch_freelist(&empty_block_buffer_freelist);
}

/* 释放一块暂时不用的block缓存 */
static inline void release_block_buffer(void *buf)
{
    add_freelist(&empty_block_buffer_freelist, buf);
}

/* 给定分区起始扇区号和blkgrp下标，返回blkgrp head所在扇区号 */
static inline afs_sector_index block_group_index_to_head_sector(
    afs_sector_index begin, afs_blkgroup_index idx)
{
    return begin + 1 + AFS_BLOCK_SECTOR_COUNT *
                       idx * (AFS_BLOCK_GROUP_MAX_EMPTY_BLOCKS_COUNT + 1);
}

void init_afs(void)
{
    init_freelist(&empty_afs_handle_freelist);
    init_freelist(&empty_block_buffer_freelist);
}

static inline size_t ceil_int_div(size_t a, size_t b)
{
    return (a / b) + (a % b ? 1 : 0);
}

/*
    格式化分区
    1. 计算各block group位置
    2. 填充每个block group head
    3. 构造根目录文件
    4. 填充分区头部
    这个过程是开中断的，分区模块有义务保证这段时间内不会有文件系统来访问该分区
*/
struct afs_handle *afs_construct(uint32_t dp_index, const struct dpt_unit *dpt)
{
    intr_state intr_s = fetch_and_enable_intr();

    ASSERT_S(dpt->type != DISK_PT_NONEXISTENT);
    ASSERT_S(dpt->sector_begin < dpt->sector_end);
    
    // 分区开头一个扇区是分区头
    // 之后的一个block group至少两个block（一个blkgrp head加上一个内容block）
    // 所以分区扇区数不能小于这些加起来，不然根目录都放不下
    if(dpt->sector_end - dpt->sector_begin <
        1 + AFS_BLOCK_SECTOR_COUNT * (1 + 1 + 1))
        return NULL;
    
    /////////////////// 填充block group heads
    
    // 跳过开头的分区头所占用的扇区
    afs_sector_index sec_offset = dpt->sector_begin + 1;
    // 上一个block group下标
    afs_blkgroup_index last_blkgrp_idx = (afs_blkgroup_index)-1;

    // 当前block group下标
    afs_blkgroup_index cur_blkgrp_idx = 0;

    // 首个block group中要拿一个block初始化为根目录文件
    afs_sector_index root_dir_sec = 0;

    while(sec_offset < dpt->sector_end)
    {
        // 还剩多少block可用
        uint32_t remain_blocks = (dpt->sector_end - sec_offset) / AFS_BLOCK_SECTOR_COUNT;
        if(remain_blocks < 2)
            break;

        struct afs_block_group_head *head =
            (struct afs_block_group_head *)alloc_block_buffer();

        // 本block group有多少block
        // 一个block group需要占用
        //  AFS_BLOCK_GROUP_MAX_EMPTY_BLOCKS_COUNT + 1
        // 个block
        head->total_blocks = AFS_BLOCK_GROUP_MAX_EMPTY_BLOCKS_COUNT;
        if(head->total_blocks + 1 > remain_blocks)
            head->total_blocks = remain_blocks - 1;

        // 如果是首个block，就拿出一个block来初始化为根目录文件
        if(cur_blkgrp_idx == 0)
        {
            head->total_blocks--;
            root_dir_sec = sec_offset +
                AFS_BLOCK_SECTOR_COUNT * (head->total_blocks + 1);
        }
        
        head->empty_blocks = head->total_blocks;
        head->next_block_group = last_blkgrp_idx;

        // 如果还有剩余空间，填在block group内部的空闲块链表内，且将该block加入blkgrp链表中
        // 否则，将该block group的空闲块链表入口置为-1
        if(head->total_blocks > 0)
        {
            head->empty_block_freelist_entry = 0;
            for(size_t i = 0;i < head->total_blocks - 1; ++i)
                head->blocks[i].next = i + 1;
            head->blocks[head->total_blocks - 1].next = -1;

            last_blkgrp_idx = cur_blkgrp_idx;
        }
        else
            head->empty_block_freelist_entry = -1;
        cur_blkgrp_idx++;

        struct disk_rw_task task =
        {
            .type           = DISK_RW_TASK_TYPE_WRITE,
            .sector_base    = sec_offset,
            .sector_cnt     = AFS_BLOCK_SECTOR_COUNT,
            .addr.write_src = head
        };
        disk_rw_raw(&task);

        sec_offset += (1 + head->total_blocks) * AFS_BLOCK_SECTOR_COUNT;

        release_block_buffer(head);
    }
    
    /////////////////// 填充根目录块

    uint32_t root_dir_byte_size =
        sizeof(uint32_t) + 2 * sizeof(struct afs_dir_entry);

    {
        char *root_dir_buf = alloc_block_buffer();
        *(uint32_t*)root_dir_buf = 2;

        struct afs_dir_entry *ety = (struct afs_dir_entry*)(root_dir_buf + sizeof(uint32_t));
        strcpy(ety->name, "..");
        ety->entry.byte_size  = root_dir_byte_size;
        ety->entry.used_size  = AFS_BLOCK_BYTE_SIZE;
        ety->entry.sector     = root_dir_sec;
        ety->entry.is_index   = 0;
        ety->entry.type       = AFS_FILE_TYPE_DIRECTORY;
        ety->entry.write_lock = 0;
        ety->entry.read_lock  = 0;

        ++ety;

        strcpy(ety->name, ".");
        ety->entry.byte_size  = root_dir_byte_size;
        ety->entry.used_size  = AFS_BLOCK_BYTE_SIZE;
        ety->entry.sector     = root_dir_sec;
        ety->entry.is_index   = 0;
        ety->entry.type       = AFS_FILE_TYPE_DIRECTORY;
        ety->entry.write_lock = 0;
        ety->entry.read_lock  = 0;

        struct disk_rw_task root_dir_task =
        {
            .type           = DISK_RW_TASK_TYPE_WRITE,
            .sector_base    = root_dir_sec,
            .sector_cnt     = AFS_BLOCK_SECTOR_COUNT,
            .addr.write_src = root_dir_buf
        };
        disk_rw_raw(&root_dir_task);

        release_block_buffer(root_dir_buf);
    }

    /////////////////// 填充分区头部扇区
    {
        // 本来只需要一个扇区，但是缓冲区申请是block为单位的，所以暂时申请一个block来用
        struct afs_dp_head *dp_head = alloc_block_buffer();

        dp_head->begin_sector = dpt->sector_begin;
        dp_head->end_sector   = dpt->sector_end;
        dp_head->fst_available_blkgrp = last_blkgrp_idx;

        dp_head->root_dir.byte_size  = root_dir_byte_size;
        dp_head->root_dir.used_size  = AFS_BLOCK_BYTE_SIZE;
        dp_head->root_dir.sector     = root_dir_sec;
        dp_head->root_dir.is_index   = 0;
        dp_head->root_dir.type       = AFS_FILE_TYPE_DIRECTORY;
        dp_head->root_dir.write_lock = 0;
        dp_head->root_dir.read_lock  = 0;

        struct disk_rw_task dp_head_task =
        {
            .type           = DISK_RW_TASK_TYPE_WRITE,
            .sector_base    = dpt->sector_begin,
            .sector_cnt     = 1,
            .addr.write_src = dp_head
        };
        disk_rw_raw(&dp_head_task);

        release_block_buffer(dp_head);
    }

    /////////////////// 创建分区handle

    struct afs_handle *handle    = alloc_afs_handle();
    handle->dp_index             = dp_index;
    handle->sector_begin         = dpt->sector_begin;
    handle->sector_end           = dpt->sector_end;
    handle->root_dir_sec         = root_dir_sec;

    if(last_blkgrp_idx != (afs_blkgroup_index)-1)
    {
        afs_sector_index fst_blkgrp_sec =
            block_group_index_to_head_sector(dpt->sector_begin, last_blkgrp_idx);
        handle->fst_available_blkgrp = alloc_block_buffer();
        struct disk_rw_task task =
        {
            .type = DISK_RW_TASK_TYPE_READ,
            .sector_base = fst_blkgrp_sec,
            .sector_cnt = AFS_BLOCK_SECTOR_COUNT,
            .addr.read_dst = handle->fst_available_blkgrp
        };
        disk_rw_raw(&task);
    }
    else
        handle->fst_available_blkgrp = NULL;

    set_intr_state(intr_s);
    return handle;
}
