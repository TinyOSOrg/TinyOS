#include <kernel/asm.h>
#include <kernel/boot.h>
#include <kernel/interrupt.h>
#include <kernel/memory.h>
#include <kernel/print.h>
#include <kernel/process/thread.h>
#include <kernel/rlist_node_alloc.h>

#include <lib/freelist.h>
#include <lib/ptrlist.h>
#include <lib/string.h>

/* 线程状态：运行，就绪，阻塞 */
enum thread_state
{
    thread_state_running,
    thread_state_ready,
    thread_state_blocked
};

/*
    thread control block
    并不是task control block
*/
struct TCB
{
    // 每个线程都持有一个内核栈
    // 在“低特权级 -> 高特权级”以及刚进入线程时会用到
    void *ker_stack;

    enum thread_state state;
};

/*
    线程被中断时，压入栈中的内容布局
    该结构体高度依赖于interrupt.s中的压栈操作
*/
struct thread_intr_bak
{
    // 最后压入的是中断向量号
    uint32_t intr_number;

    // 通用寄存器
    uint32_t edi, esi, ebp, esp_dummy;
    uint32_t ebx, edx, ecx, eax;

    // 段选择子
    // 虽然应该是16位的，但压栈的结果是32位，只用低16位即可
    uint32_t gs, fs, es, ds;

    // 中断恢复相关，由CPU自动压栈的内容
    uint32_t err_code; // 有的中断有错误码而有的没有，interrupt.s中将其统一为有
    uint32_t eip;      // C标准不保证函数指针能放到uint32_t中，但x86下没问题
    uint32_t cs;
    uint32_t eflags;   // 标志寄存器
    uint32_t esp;
    uint32_t ss;
};

#define INTR_BAK_DATA_SIZE 76

/*
    线程栈初始化时栈顶的内容
    这个结构参考了真像还原一书
    利用ret指令及其参数在栈中分布的特点来实现控制流跨线程跳转
*/
struct thread_init_stack
{
    // 由callee保持的寄存器
    uint32_t ebp, ebx, edi, esi;

    // 函数入口
    uint32_t eip;

    // 占位，因为需要一个返回地址的空间
    uint32_t dummy_addr;

    // 线程入口及参数
    thread_exec_func func;
    void *func_param;
};

/* 空闲TCB自由链表 */
static freelist_handle TCB_freelist;

/* 当前正在运行的线程 */
static struct TCB *cur_running_TCB;

/* ready线程队列 */
static rlist ready_threads;

/* 分配一个空TCB块 */
static struct TCB *alloc_TCB(void)
{
    // 没有空余TCB时，分配一个新的内核页并加入自由链表
    if(is_freelist_empty(&TCB_freelist))
    {
        struct TCB *new_TCBs = (struct TCB*)alloc_ker_page(true);
        size_t end = 4096 / sizeof(struct TCB);
        for(size_t i = 0;i != end; ++i)
            add_freelist(&TCB_freelist, &new_TCBs[i]);
    }

    return fetch_freelist(&TCB_freelist);
}

/*
    进入一个线程的前导操作
    有什么初始化啥的就在这儿做
*/
static void kernel_thread_entry(thread_exec_func func, void *params)
{
    // 线程的第一次进入也是通过调度器进来的
    // 此时中断为关闭状态，所以刚进入线程时开一下中断
    _enable_intr();

    func(params);
}

/* 从开机以来就一直在跑的家伙也是个线程 */
static void init_bootloader_thread(void)
{
    struct TCB *tcb = alloc_TCB();
    tcb->state = thread_state_running;
    tcb->ker_stack = (void*)KER_STACK_INIT_VAL;
    cur_running_TCB = tcb;
}

/*
    线程调度
    现在就单纯地从把当前running的线程换到ready，然后从ready中取出一个变成running
*/
static void thread_scheduler(void)
{
    // 从src线程切换至dst线程
    // 在thread.s中实现
    extern void switch_to_thread(
            struct TCB *src, struct TCB *dst);

    push_back_rlist(&ready_threads, cur_running_TCB,
                    kernel_resident_rlist_node_alloc);
    cur_running_TCB->state = thread_state_ready;

    struct TCB *last = cur_running_TCB;
    cur_running_TCB = pop_front_rlist(&ready_threads,
                        kernel_resident_rlist_node_dealloc);
    cur_running_TCB->state = thread_state_running;
    
    switch_to_thread(last, cur_running_TCB);
}

void init_thread_man(void)
{
    init_freelist(&TCB_freelist);

    init_rlist(&ready_threads);

    init_bootloader_thread();

    set_intr_function(INTR_NUMBER_CLOCK, thread_scheduler);
}

struct TCB *create_thread(thread_exec_func func, void *params)
{
    intr_state intr_s = get_intr_state();
    _disable_intr();

    // 分配TCB和内核栈空间
    
    struct TCB *tcb = alloc_TCB();
    tcb->state = thread_state_ready;
    
    tcb->ker_stack = alloc_ker_page(true);
    memset((char*)tcb->ker_stack, 0x0, 4096);
    tcb->ker_stack = (char*)tcb->ker_stack + 4096
                   - INTR_BAK_DATA_SIZE
                   - sizeof(struct thread_init_stack);

    // 初始化内核栈顶端信息

    struct thread_init_stack *init_stack =
        (struct thread_init_stack*)tcb->ker_stack;
    init_stack->eip = (uint32_t)kernel_thread_entry;
    init_stack->func = func;
    init_stack->func_param = params;
    init_stack->ebp = init_stack->ebx = 0;
    init_stack->esi = init_stack->edi = 0;

    // 加入ready队列
    push_back_rlist(&ready_threads, tcb,
        kernel_resident_rlist_node_alloc);

    // 还原中断状态
    set_intr_state(intr_s);

    return tcb;
}
