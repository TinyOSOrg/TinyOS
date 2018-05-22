#include <kernel/assert.h>
#include <kernel/diskdriver.h>
#include <kernel/filesys/dpt.h>
#include <kernel/memory.h>

#include <kernel/filesys/afs/afs.h>

#include <shared/filesys/dpt.h>
#include <shared/string.h>

#define DPT_BYTE_SIZE (sizeof(struct dpt_unit) * DPT_UNIT_COUNT)

/* 分区表内容 */
static struct dpt_unit *dpts;

/* 分区fs handlers */
static uint32_t dp_fs_handles[DPT_UNIT_COUNT];

void init_dpt()
{
    // 这里直接申请一个扇区的大小，后面那点零头不用在意，主要是重写dpt的时候方便
    dpts = alloc_static_kernel_mem(512, sizeof(struct dpt_unit));
    
    // dpt_sec_data在内核进程栈上，不用担心缺页，所以直接无缓冲读取
    uint8_t dpt_sec_data[512];
    disk_read(DPT_SECTOR_POSITION, 1, dpt_sec_data);
    memcpy((char*)dpts, (const char*)dpt_sec_data, DPT_BYTE_SIZE);

    for(size_t i = 0;i != DPT_UNIT_COUNT; ++i)
    {
        struct dpt_unit *u = &dpts[i];
        switch(u->type)
        {
        case DISK_PT_AFS:
            dp_fs_handles[i] = (uint32_t)afs_init_dp_handler(u->sector_begin);
            break;
        default:
            dp_fs_handles[i] = 0;
        }
    }
}

struct dpt_unit *get_dpt_unit(size_t idx)
{
    ASSERT_S(0 <= idx && idx < DPT_UNIT_COUNT);
    return &dpts[idx];
}

bool reformat_dp(size_t idx, disk_partition_type type)
{
    // 先销毁原来的文件系统handler
    struct dpt_unit *u = &dpts[idx];
    switch(u->type)
    {
    case DISK_PT_AFS:
        afs_release_dp_handler(dp_fs_handles[idx]);
        break;
    }

    switch(type)
    {
    case DISK_PT_AFS:
        if(!afs_reformat_dp(u->sector_begin, u->sector_end - u->sector_begin))
        {
            u->type = DISK_PT_NOFS;
            return false;
        }
        dp_fs_handles[idx] = (uint32_t)afs_init_dp_handler(u->sector_begin);
    }

    u->type = type;
    return true;
}

uint32_t get_dp_fs_handler(size_t idx)
{
    return dp_fs_handles[idx];
}

void restore_dpt()
{
    disk_write(DPT_SECTOR_POSITION, 1, dpts);
}

void destroy_dpt()
{
    for(uint32_t i = 0; i < DPT_UNIT_COUNT; ++i)
    {
        struct dpt_unit *u = get_dpt_unit(i);
        switch(u->type)
        {
        case DISK_PT_AFS:
            afs_release_dp_handler(get_dp_fs_handler(i));
            break;
        }
    }

    restore_dpt();
}
