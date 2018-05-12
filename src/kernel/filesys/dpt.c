#include <kernel/assert.h>
#include <kernel/diskdriver.h>
#include <kernel/filesys/dpt.h>
#include <kernel/memory.h>

#include <shared/filesys/dpt.h>
#include <shared/string.h>

#define DPT_BYTE_SIZE (sizeof(struct dpt_unit) * DPT_UNIT_COUNT)

static struct dpt_unit *dpts;

void init_dpt()
{
    // 这里直接申请一个扇区的大小，后面那点零头不用在意，主要是重写dpt的时候方便
    dpts = alloc_static_kernel_mem(512, sizeof(struct dpt_unit));
    
    // dpt_sec_data在内核进程栈上，不用担心缺页，所以直接无缓冲读取
    uint8_t dpt_sec_data[512];
    struct disk_rw_task dpt_sec_task =
    {
        .type          = DISK_RW_TASK_TYPE_READ,
        .sector_base   = DPT_SECTOR_POSITION,
        .sector_cnt    = 1,
        .addr.read_dst = dpt_sec_data
    };
    disk_rw_raw(&dpt_sec_task);
    memcpy((char*)dpts, (char*)dpt_sec_data, DPT_BYTE_SIZE);
}

struct dpt_unit *get_dpt_unit(size_t idx)
{
    ASSERT_S(0 <= idx && idx < DPT_UNIT_COUNT);
    return &dpts[idx];
}

void restore_dpt()
{
    struct disk_rw_task dpt_sec_task =
    {
        .type           = DISK_RW_TASK_TYPE_WRITE,
        .sector_base    = DPT_SECTOR_POSITION,
        .sector_cnt     = 1,
        .addr.write_src = dpts
    };
    disk_rw_raw(&dpt_sec_task);
}
