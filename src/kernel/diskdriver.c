#include <kernel/asm.h>
#include <kernel/assert.h>
#include <kernel/diskdriver.h>
#include <kernel/interrupt.h>
#include <kernel/process/thread.h>

#include <shared/string.h>

/* IDE0相关的控制端口号 */
#define DISK_PORT_DATA         0x1f0
#define DISK_PORT_ERROR        0x1f1
#define DISK_PORT_SECTOR_COUNT 0x1f2
#define DISK_PORT_LBA_LOW      0x1f3
#define DISK_PORT_LBA_MID      0x1f4
#define DISK_PORT_LBA_HIG      0x1f5
#define DISK_PORT_DEVICE       0x1f6
#define DISK_PORT_STATUS       0x1f7
#define DISK_PORT_CMD          0x1f7

/* status忙等待位 */
#define DISK_STATUS_BUSY  0x80
/* 是否可以进行数据传输 */
#define DISK_STATUS_READY 0x8

#define IS_DISK_BUSY() \
    ((_in_byte_from_port(DISK_PORT_STATUS) & DISK_STATUS_BUSY) != 0)
#define IS_DISK_READY() \
    ((_in_byte_from_port(DISK_PORT_STATUS) & DISK_STATUS_READY) != 0)

/* 阻塞在pf_flag上的缺页任务的任务描述 */
static struct disk_rw_task pf_task;

/* 硬盘忙标志 */
static struct TCB * volatile busy_flag;
/* 缺页标志 */
static struct TCB * volatile pf_flag;

void init_disk_driver(void)
{
    busy_flag = NULL;
    pf_flag   = NULL;
}

static inline bool wait_for_ready(void)
{
    while(IS_DISK_BUSY())
        ;
    return IS_DISK_READY();
}

/* 向硬盘提交一个任务命令 */
static void disk_cmd(const struct disk_rw_task *task)
{
    // Sector count
    ASSERT_S(task->sector_cnt < 256);
    _out_byte_to_port(DISK_PORT_SECTOR_COUNT, (uint8_t)task->sector_cnt);

    // LBA low/mid/high
    _out_byte_to_port(DISK_PORT_LBA_LOW, (uint8_t)(task->sector_base & 0xff));
    _out_byte_to_port(DISK_PORT_LBA_MID, (uint8_t)((task->sector_base >> 8) & 0xff));
    _out_byte_to_port(DISK_PORT_LBA_HIG, (uint8_t)((task->sector_base >> 16) & 0xff));

    // Device
    _out_byte_to_port(DISK_PORT_DEVICE, (uint8_t)(0x40 | 0xa0 | (task->sector_base >> 24)));

    // Command
    _out_byte_to_port(DISK_PORT_CMD,
                      task->type == DISK_RW_TASK_TYPE_READ ? 0x20 : 0x30);
}

static void commit_pf_task(void)
{
    ASSERT_S(pf_flag && !busy_flag);
    disk_cmd(&pf_task);
    busy_flag = pf_flag;
    pf_flag   = NULL;
}

static void post_post_process(void)
{
    ASSERT_S(busy_flag == NULL);
    if(pf_flag)
    {
        commit_pf_task();
        ASSERT_S(busy_flag);
    }
}

static bool try_disk_rw_raw(const struct disk_rw_task *task)
{
    if(busy_flag)
    {
        yield_CPU();
        return false;
    }

    if(pf_flag)
    {
        commit_pf_task();
        return false;
    }

    disk_cmd(task);

    if(task->type == DISK_RW_TASK_TYPE_READ)
    {
        busy_flag = get_cur_TCB();
        block_cur_thread();
        if(!wait_for_ready())
            FATAL_ERROR("invalid disk status");
        _in_words_from_port(DISK_PORT_DATA, task->addr.read_dst, task->sector_cnt * 512 / 2);
    }
    else
    {
        if(!wait_for_ready())
            FATAL_ERROR("invalid disk status");
        _out_words_to_port(DISK_PORT_DATA, task->addr.write_src, task->sector_cnt * 512 / 2);
        busy_flag = get_cur_TCB();
        block_cur_thread();
    }

    post_post_process();
    return true;
}

static bool try_disk_pfrw_raw(const struct disk_rw_task *task)
{
    if(busy_flag)
    {
        if(pf_flag)
        {
            yield_CPU();
            return false;
        }
        else
        {
            pf_flag = get_cur_TCB();
            memcpy((char*)&pf_task, (char*)task, sizeof(struct disk_rw_task));
            block_cur_thread();
            return false;
        }
    }

    if(pf_flag)
    {
        commit_pf_task();
        pf_flag = get_cur_TCB();
        memcpy((char*)&pf_task, (char*)task, sizeof(struct disk_rw_task));
        block_cur_thread();
        return false;
    }

    disk_cmd(task);

    if(task->type == DISK_RW_TASK_TYPE_READ)
    {
        busy_flag = get_cur_TCB();
        block_cur_thread();
        if(!wait_for_ready())
            FATAL_ERROR("invalid disk status");
        _in_words_from_port(DISK_PORT_DATA, task->addr.read_dst, task->sector_cnt * 512 / 2);
    }
    else
    {
        if(!wait_for_ready())
            FATAL_ERROR("invalid disk status");
        _out_words_to_port(DISK_PORT_DATA, task->addr.write_src, task->sector_cnt * 512 / 2);
        busy_flag = get_cur_TCB();
        block_cur_thread();
    }

    post_post_process();
    return true;
}

void disk_rw_raw(const struct disk_rw_task *task)
{
    intr_state intr_s = fetch_and_disable_intr();
    while(!try_disk_rw_raw(task))
        ;
    set_intr_state(intr_s);
}

void disk_pfrw_raw(const struct disk_rw_task *task)
{
    intr_state intr_s = fetch_and_disable_intr();
    while(!try_disk_pfrw_raw(task))
        ;
    set_intr_state(intr_s);
}
