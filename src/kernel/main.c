#include <kernel/asm.h>
#include <kernel/assert.h>
#include <kernel/interrupt.h>
#include <kernel/memory.h>
#include <kernel/console/print.h>
#include <kernel/console/console.h>
#include <kernel/diskdriver.h>
#include <kernel/filesys/dpt.h>
#include <kernel/kbdriver.h>
#include <kernel/process/semaphore.h>
#include <kernel/process/process.h>
#include <kernel/rlist_node_alloc.h>
#include <kernel/syscall.h>
#include <kernel/sysmsg/sysmsg_syscall.h>
#include <kernel/sysmsg/sysmsg.h>

#include <lib/conio.h>

#include <shared/keycode.h>
#include <shared/rbtree.h>
#include <shared/string.h>
#include <shared/syscall/common.h>
#include <shared/syscall/sysmsg.h>
#include <shared/sysmsg/kbmsg.h>

#define syscall_param0(N) \
    ({ uint32_t r; \
       asm volatile ("int $0x80;" \
                     : "=a" (r) \
                     : "a" (N) \
                     : "memory"); \
       r; })

#define syscall_param1(N, arg1) \
    ({ uint32_t r; \
       asm volatile ("int $0x80" \
                     : "=a" (r) \
                     : "a" (N), "b" (arg1) \
                     : "memory"); \
       r; })

#define syscall_param2(N, arg1, arg2) \
    ({ uint32_t r; \
       asm volatile ("int $0x80" \
                     : "=a" (r) \
                     : "a" (N), "b" (arg1), "c" (arg2) \
                     : "memory"); \
       r; })

#define syscall_param3(N, arg1, arg2, arg3) \
    ({ uint32_t r; \
       asm volatile ("int $0x80" \
                     : "=a" (r) \
                     : "a" (N), "b" (arg1), "c" (arg2), "d" (arg3) \
                     : "memory"); \
       r; })

void PL0_thread(void)
{
    printf("hahaha process, pid = %u\n",
        syscall_param0(SYSCALL_GET_PROCESS_ID));
    syscall_param1(SYSCALL_SYSMSG_OPERATION, SYSMSG_SYSCALL_FUNCTION_REGISTER_CHAR_MSG);
    while(1)
    {
        syscall_param1(SYSCALL_SYSMSG_OPERATION,
                       SYSMSG_SYSCALL_FUNCTION_BLOCK_ONTO_SYSMSG);
        struct sysmsg msg;
        while(syscall_param3(SYSCALL_SYSMSG_OPERATION,
                          SYSMSG_SYSCALL_FUNCTION_PEEK_MSG,
                          SYSMSG_SYSCALL_PEEK_OPERATION_REMOVE,
                          &msg))
        {
            if(msg.type == SYSMSG_TYPE_CHAR)
            {
                struct kbchar_msg_struct *chmsg = (struct kbchar_msg_struct*)&msg;
                put_char(chmsg->ch);
            }
        }
    }
    exit_thread();
}

/* 红黑树测试 */
void PL1_thread_rbtree(void)
{
    struct rb_data
    {
        int key;
        struct rb_node node;
    };

#define KOF RB_MEM_TO_MEM_OFFSET(struct rb_data, node, key)

    bool rb_data_less(const void *L, const void *R)
    {
        return *(int*)L < *(int*)R;
    }

    printf("rbtree test begin\n");

    struct rb_tree T;
    rb_init(&T);

    int key = 5;
    struct rb_data *nodes = (struct rb_data*)alloc_ker_page(false);
    ASSERT_S(rb_find(&T, KOF, &key, rb_data_less) == NULL);

    nodes[0].key = 2;
    rb_insert(&T, &nodes[0].node, KOF, rb_data_less);

    nodes[1].key = 7;
    rb_insert(&T, &nodes[1].node, KOF, rb_data_less);

    nodes[2].key = 5;
    rb_insert(&T, &nodes[2].node, KOF, rb_data_less);

    key = 2;
    ASSERT_S(rb_find(&T, KOF, &key, rb_data_less) == &nodes[0].node);

    key = 5;
    ASSERT_S(rb_find(&T, KOF, &key, rb_data_less) == &nodes[2].node);

    key = 3;
    ASSERT_S(rb_find(&T, KOF, &key, rb_data_less) == NULL);

    rb_erase(&T, &nodes[2].node, KOF, rb_data_less);

    key = 5;
    ASSERT_S(rb_find(&T, KOF, &key, rb_data_less) == NULL);

    key = 2;
    ASSERT_S(rb_find(&T, KOF, &key, rb_data_less) == &nodes[0].node);

    rb_erase(&T, &nodes[0].node, KOF, rb_data_less);

    key = 2;
    ASSERT_S(rb_find(&T, KOF, &key, rb_data_less) == NULL);

    key = 7;
    ASSERT_S(rb_find(&T, KOF, &key, rb_data_less) == &nodes[1].node);

    free_ker_page(nodes);

    printf("rbtree test end\n");

    exit_thread();
}

void init_kernel(void)
{
    /* 中断系统 */
    init_IDT();

    /* 物理内存 */
    init_phy_mem_man();

    /* 虚拟内存 */
    init_vir_mem_man();

    /* 内核链表分配 */
    init_kernel_rlist_node_alloc();

    /* 内核线程 */
    init_thread_man();

    /* 进程管理 */
    init_process_man();

    /* 系统调用 */
    init_syscall();

    /* 控制台 */
    init_console();

    /* 内核消息传递 */
    init_sysmsg();

    /* 内核消息系统调用 */
    init_sysmsg_syscall();

    /* 时钟中断频率 */
    set_8253_freq(100);

    /* 键盘驱动 */
    init_kb_driver();

    /* 硬盘驱动 */
    init_disk_driver();

    /* 磁盘分区 */
    init_dpt();
}

int main(void)
{
    init_kernel();

    set_cursor_row_col(0, 0);

    create_process("another process", PL0_thread, true);

    create_process("rbtree process", PL1_thread_rbtree, true);

    _enable_intr();

    printf("main process, pid = %u\n", syscall_param0(SYSCALL_GET_PROCESS_ID));

    while(1)
    {
        do_releasing_thds_procs();
        yield_CPU();
    }
}
