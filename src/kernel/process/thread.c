#include <kernel/asm.h>
#include <kernel/assert.h>
#include <kernel/boot.h>
#include <kernel/interrupt.h>
#include <kernel/memory.h>
#include <kernel/process/process.h>
#include <kernel/process/semaphore.h>
#include <kernel/process/thread.h>
#include <kernel/rlist_node_alloc.h>
#include <kernel/sysmsg/sysmsg.h>
#include <kernel/sysmsg/sysmsg_src.h>

#include <shared/freelist.h>
#include <shared/ptrlist.h>
#include <shared/string.h>

/*
    中断发生时压入栈中的东西得占多少字节
    事实上有这么些东西：

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
*/
#define INTR_BAK_DATA_SIZE 76

/* 空闲TCB自由链表 */
static freelist_handle TCB_freelist;

/* 当前正在运行的线程 */
static struct TCB *cur_running_TCB;

/* ready线程队列 */
static ilist ready_threads;

/* 待释放的tcb */
static rlist waiting_release_threads;

/* 待释放的pcb */
static rlist waiting_release_processes;

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

/* 进入一个线程的前导操作 */
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
    tcb->init_ker_stack = tcb->ker_stack;
    tcb->pcb = NULL;
    tcb->blocked_sph = NULL;
    cur_running_TCB = tcb;
}

/*
    从进程的线程表中干掉一个线程
    如果是该进程的唯一一个线程，那么这个进程要干掉
    必须要保证：tcb不是running，也不在任何ready队列中
*/
static void erase_thread_in_process(struct TCB *tcb)
{
    intr_state intr_s = fetch_and_disable_intr();

    struct PCB *pcb = tcb->pcb;

    if(tcb == pcb->sysmsg_blocked_tcb)
        pcb->sysmsg_blocked_tcb = NULL;
    
    // 从进程的线程表中删除该线程
    // 并检查是否需要干掉进程
    // 因为虚拟地址空间现在还用着呢，所以先不去掉它，放入waiting_release_processes队列等会儿再做
    erase_from_ilist(&tcb->threads_in_proc_node);
    if(is_ilist_empty(&pcb->threads_list))
    {
        _clr_PID_to_PCB(pcb->pid);
        erase_from_ilist(&pcb->processes_node);
        destroy_sysmsg_queue(&pcb->sys_msgs);
        destroy_sysmsg_source_list(&pcb->sys_msg_srcs);

        push_back_rlist(&waiting_release_processes,
            pcb, kernel_resident_rlist_node_alloc);
    }

    // tcb本身占的资源也要释放了，但现在不太合适，这个内核栈还在用呢……
    // 所以延迟到之后进行
    push_back_rlist(&waiting_release_threads,
        tcb, kernel_resident_rlist_node_alloc);
    
    set_intr_state(intr_s);
}

/*
    线程调度
    现在就单纯地从把当前running的线程换到ready，然后从ready中取出一个变成running
*/
static void thread_scheduler(void)
{
    // 从src线程切换至dst线程
    // 在thread.s中实现
    extern void switch_to_thread(struct TCB *src, struct TCB *dst);
    extern void jmp_to_thread(struct TCB *dst);
    
    if(cur_running_TCB->state == thread_state_running)
    {
        push_back_ilist(&ready_threads, &cur_running_TCB->ready_block_threads_node);
        cur_running_TCB->state = thread_state_ready;
    }

    ASSERT_S(!is_ilist_empty(&ready_threads));

    struct TCB *last = cur_running_TCB;
    cur_running_TCB = GET_STRUCT_FROM_MEMBER(struct TCB, ready_block_threads_node,
                                             pop_front_ilist(&ready_threads));
    cur_running_TCB->state = thread_state_running;

    if(last->pcb != cur_running_TCB->pcb)
    {
        set_current_vir_addr_space(cur_running_TCB->pcb->addr_space);
        if(!cur_running_TCB->pcb->is_PL_0)
            _set_tss_esp0((uint32_t)(cur_running_TCB->init_ker_stack));
    }

    if(last->state == thread_state_killed)
    {
        erase_thread_in_process(last);
        jmp_to_thread(cur_running_TCB);
    }
    else
    {
        switch_to_thread(last, cur_running_TCB);
    }
}

void init_thread_man(void)
{
    init_freelist(&TCB_freelist);

    init_ilist(&ready_threads);

    init_rlist(&waiting_release_threads);
    init_rlist(&waiting_release_processes);

    init_bootloader_thread();

    set_intr_function(INTR_NUMBER_CLOCK, thread_scheduler);
}

struct TCB *create_thread(thread_exec_func func, void *params,
                          struct PCB *pcb)
{
    intr_state intr_s = fetch_and_disable_intr();

    // 分配TCB和内核栈空间
    
    struct TCB *tcb = alloc_TCB();
    tcb->pcb = pcb;
    tcb->state = thread_state_ready;
    tcb->blocked_sph = NULL;
    
    tcb->ker_stack = alloc_ker_page(true);
    memset((char*)tcb->ker_stack, 0x0, 4096);
    tcb->ker_stack = (char*)tcb->ker_stack + 4096
                   - INTR_BAK_DATA_SIZE
                   - sizeof(struct thread_init_stack);
    tcb->init_ker_stack = tcb->ker_stack;

    // 初始化内核栈顶端信息

    struct thread_init_stack *init_stack =
        (struct thread_init_stack*)tcb->ker_stack;
    init_stack->eip = (uint32_t)kernel_thread_entry;
    init_stack->func = func;
    init_stack->func_param = params;
    init_stack->ebp = init_stack->ebx = 0;
    init_stack->esi = init_stack->edi = 0;

    // 加入ready队列
    push_back_ilist(&ready_threads, &tcb->ready_block_threads_node);

    // 还原中断状态
    set_intr_state(intr_s);

    return tcb;
}

struct TCB *get_cur_TCB(void)
{
    return cur_running_TCB;
}

void block_cur_thread(void)
{
    intr_state intr_s = fetch_and_disable_intr();

    cur_running_TCB->state = thread_state_blocked;
    thread_scheduler();
    
    set_intr_state(intr_s);
}

void block_cur_thread_onto_sysmsg(void)
{
    intr_state intr_s = fetch_and_disable_intr();

    cur_running_TCB->state = thread_state_blocked;
    cur_running_TCB->pcb->sysmsg_blocked_tcb = cur_running_TCB;
    thread_scheduler();
    
    set_intr_state(intr_s);
}

void awake_thread(struct TCB *tcb)
{
    intr_state intr_s = fetch_and_disable_intr();

    ASSERT_S(tcb->state == thread_state_blocked);
    tcb->state = thread_state_ready;
    push_back_ilist(&ready_threads, &tcb->ready_block_threads_node);

    set_intr_state(intr_s);
}

void kill_thread(struct TCB *tcb)
{
    intr_state intr_s = fetch_and_disable_intr();

    ASSERT_S(tcb->state != thread_state_killed);

    if(tcb == cur_running_TCB)
    {
        // 如果被干掉的线程是当前正在跑的，那么需要调度器
        tcb->state = thread_state_killed;
        thread_scheduler();
    }
    else // 在ready队列中或被阻塞
    {
        erase_from_ilist(&tcb->ready_block_threads_node);
        erase_thread_in_process(tcb);
    }

    set_intr_state(intr_s);
}

void exit_thread(void)
{
    kill_thread(cur_running_TCB);
}

void do_releasing_thds_procs(void)
{
    intr_state intr_s = fetch_and_disable_intr();

    // 线程
    while(!is_rlist_empty(&waiting_release_threads))
    {
        struct TCB *tcb = pop_front_rlist(&waiting_release_threads,
            kernel_resident_rlist_node_dealloc);
        // 内核栈
        uint32_t ker_stk_page = (uint32_t)tcb->init_ker_stack
                                        + INTR_BAK_DATA_SIZE
                                        + sizeof(struct thread_init_stack)
                                        - 4096;
        ASSERT_S(ker_stk_page % 4096 == 0);
        free_ker_page((char*)ker_stk_page);
        // 信号量
        if(tcb->blocked_sph)
            tcb->blocked_sph->val++;

        // tcb空间
        add_freelist(&TCB_freelist, tcb);
    }

    // 进程
    while(!is_rlist_empty(&waiting_release_processes))
    {
        struct PCB *pcb = pop_front_rlist(&waiting_release_processes,
            kernel_resident_rlist_node_dealloc);
        // 虚拟地址空间
        destroy_vir_addr_space(pcb->addr_space);
        // pcb空间
        _add_PCB_mem(pcb);
    }

    set_intr_state(intr_s);
}

void yield_CPU(void)
{
    thread_scheduler();
}
