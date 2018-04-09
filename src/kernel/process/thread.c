#include <kernel/memory.h>
#include <kernel/process/thread.h>

#include <lib/freelist.h>
#include <lib/string.h>

/* 线程状态：运行，就绪，阻塞 */
enum thread_state
{
    thread_state_running,
    thread_state_ready,
    thread_state_blocked
};

/*
    thread control blocl
    并不是task control block
*/
struct TCB
{
    enum thread_state state;

    // 每个线程都持有一个内核栈
    // 在“低特权级 -> 高特权级”以及刚进入线程时会用到
    void *ker_stack;
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

/* 线程栈初始化时栈顶的内容 */
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
    有什么初始化啥的以后就在这儿做
*/
static void kernel_thread_entry(thread_exec_func func, void *params)
{
    func(params);
}

void init_thread_man(void)
{
    init_freelist(&TCB_freelist);
}

struct TCB *create_thread(thread_exec_func func, void *params)
{
    // 分配TCB和内核栈空间
    
    struct TCB *tcb = alloc_TCB();
    tcb->state = thread_state_running;
    
    tcb->ker_stack = alloc_ker_page(true);
    memset((char*)tcb->ker_stack, 0x0, 4096);
    tcb->ker_stack = (char*)tcb->ker_stack + 4096 - sizeof(struct thread_intr_bak)
                                                  - sizeof(struct thread_init_stack);

    // 初始化内核栈顶端信息

    struct thread_init_stack *init_stack = (struct thread_init_stack*)tcb->ker_stack;
    init_stack->eip = (uint32_t)&kernel_thread_entry;
    init_stack->func = func;
    init_stack->func_param = params;
    init_stack->ebp = init_stack->ebx =
    init_stack->esi = init_stack->edi = 0;

    // 控制流转移

    asm volatile
    (
        "movl %0, %%esp;"
        "pop %%ebp;"
        "pop %%ebx;"
        "pop %%edi;"
        "pop %%esi;"
        "ret"
        :
        : "g" (tcb->ker_stack)
        : "memory"
    );

    return tcb;
}
