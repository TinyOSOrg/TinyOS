#include <kernel/diskdriver.h>
#include <kernel/filesys/dpt.h>
#include <kernel/memory.h>

#include <shared/filesys/dpt.h>
#include <shared/string.h>

#define DPT_BYTE_SIZE (sizeof(struct dpt_unit) * DPT_UNIT_COUNT)

static struct dpt_unit *dpts;

void init_dpt(void)
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

void rewrite_dpt(void)
{
    struct disk_rw_task task =
    {
        .type           = DISK_RW_TASK_TYPE_WRITE,
        .sector_base    = DPT_SECTOR_POSITION,
        .sector_cnt     = 1,
        .addr.write_src = dpts
    };
    disk_rw_raw(&task);
}
