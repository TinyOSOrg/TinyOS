#include <kernel/asm.h>
#include <kernel/assert.h>
#include <kernel/driver/diskdriver.h>
#include <kernel/interrupt.h>
#include <kernel/process/thread.h>
#include <kernel/process/semaphore.h>

#include <shared/string.h>
#include <shared/utility.h>

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

/* 普通任务阻塞线程队列 */
static ilist blocked_normal_tasks;

static struct TCB *busy_task;

static inline bool wait_for_ready()
{
    while(IS_DISK_BUSY())
        ;
    return IS_DISK_READY();
}

/* 向硬盘提交一个任务命令 */
static void disk_cmd(const struct disk_rw_task *task)
{
    // Sector count
    ASSERT(task->sector_cnt < 256);
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

static void normal_task_entry()
{
    while(busy_task)
    {
        push_back_ilist(&blocked_normal_tasks,
                        &get_cur_TCB()->ready_block_threads_node);
        block_cur_thread();
    }
    busy_task = get_cur_TCB();
}

static void task_exit()
{
    busy_task = NULL;

    if(!is_ilist_empty(&blocked_normal_tasks))
    {
        struct ilist_node *next = pop_front_ilist(&blocked_normal_tasks);
        struct TCB *tcb = GET_STRUCT_FROM_MEMBER(struct TCB, ready_block_threads_node,
                                                 next);
        awake_thread(tcb);
    }
}

static void disk_intr_handler()
{
    if(busy_task)
        awake_thread(busy_task);
    _in_byte_from_port(DISK_PORT_STATUS);
}

static void disk_read_raw_impl(const struct disk_rw_task *task)
{
    normal_task_entry();

    disk_cmd(task);
    block_cur_thread();

    if(!wait_for_ready())
        FATAL_ERROR("invalid disk status");
    
    _in_words_from_port(DISK_PORT_DATA, task->addr.read_dst,
                                        task->sector_cnt * 512 / 2);

    task_exit();
}

static void disk_write_raw_impl(const struct disk_rw_task *task)
{
    normal_task_entry();

    disk_cmd(task);

    if(!wait_for_ready())
        FATAL_ERROR("invalid disk status");
    
    _out_words_to_port(DISK_PORT_DATA, task->addr.write_src,
                                       task->sector_cnt * 512 / 2);
        
    block_cur_thread();

    task_exit();
}

void init_disk_driver()
{
    busy_task = NULL;

    init_ilist(&blocked_normal_tasks);

    set_intr_function(INTR_NUMBER_DISK, disk_intr_handler);
}

void disk_rw_raw(const struct disk_rw_task *task)
{
    intr_state is = fetch_and_disable_intr();

    if(task->type == DISK_RW_TASK_TYPE_READ)
        disk_read_raw_impl(task);
    else if(task->type == DISK_RW_TASK_TYPE_WRITE)
        disk_write_raw_impl(task);

    set_intr_state(is);
}
