#include <kernel/asm.h>
#include <kernel/boot.h>
#include <kernel/interrupt.h>
#include <kernel/memory.h>
#include <kernel/process/process.h>
#include <kernel/process/thread.h>
#include <kernel/rlist_node_alloc.h>
#include <kernel/seg_desc.h>

#include <kernel/print.h>

#include <lib/freelist.h>
#include <lib/intdef.h>
#include <lib/ptrlist.h>
#include <lib/string.h>

/*=====================================================================
    用户段相关
=====================================================================*/

/* TSS段结构 */
struct TSS
{
    uint32_t BL;
    uint32_t esp0, ss0;
    uint32_t esp1, ss1;
    uint32_t esp2, ss2;
    uint32_t cr3;
    uint32_t eip;
    uint32_t eflags;
    uint32_t eax, ecx, edx, ebx;
    uint32_t esp, ebp, esi, edi;
    uint32_t es, cs, ss, ds, fs, gs;
    uint32_t LDT;
    uint32_t trace;
    uint32_t IO;
};

/*
    TSS段内容
    跟linux类似，整个系统就这一个
*/
static struct TSS tss;

/* 初始化TSS描述符 */
static void init_TSS(void)
{
    memset((char*)&tss, 0x0, sizeof(struct TSS));
    tss.ss0 = SEG_SEL_KERNEL_STACK;
    tss.IO = sizeof(struct TSS);
    make_iGDT((struct iGDT*)((char*)GDT_START + 3 * 8),
              (uint32_t)&tss, sizeof(struct TSS) - 1, 
              TSS_ATTRIB_LOW, TSS_ATTRIB_HIGH);
}

/* 初始化用户代码段和数据段描述符 */
static void init_user_segments(void)
{
    make_iGDT((struct iGDT*)((char*)GDT_START + 4 * 8),
              0, 0xfffff,
              GDT_ATTRIB_LOW_CODE, GDT_ATTRIB_HIGH);
    make_iGDT((struct iGDT*)((char*)GDT_START + 5 * 8),
              0, 0xfffff,
              GDT_ATTRIB_LOW_DATA, GDT_ATTRIB_HIGH);
}

/*=====================================================================
    进程本体相关
=====================================================================*/

struct intr_stack_bak
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

/* 进程列表 */
static ilist processes;

/* PCB空间自由链表 */
static freelist_handle PCB_freelist;

/* 申请一块PCB空间 */
static struct PCB *alloc_PCB(void)
{
    if(is_freelist_empty(&PCB_freelist))
    {
        struct PCB *new_PCBs = (struct PCB*)alloc_ker_page(true);
        size_t end = 4096 / sizeof(struct PCB);
        for(size_t i = 0;i != end; ++i)
            add_freelist(&PCB_freelist, &new_PCBs[i]);
    }
    return fetch_freelist(&PCB_freelist);
}

/* 有多少个用户栈位图 */
#define USER_THREAD_STACK_BITMAP_COUNT (MAX_PROCESS_THREADS >> 5)

/* 创建一个进程，但是不包含任何线程（ */
static struct PCB *create_empty_process(const char *name, bool is_PL_0)
{
    struct PCB *pcb = alloc_PCB();

    // 虚拟地址空间初始化

    pcb->addr_space = create_vir_addr_space();
    pcb->pid = get_usr_vir_addr_idx(pcb->addr_space) + 1;
    pcb->is_PL_0 = is_PL_0;
    pcb->addr_space_inited = false;

    init_ilist(&pcb->threads_list);

    // 名字复制，超出长度限制的部分都丢掉
    size_t i_name = 0;
    for(;i_name != PROCESS_NAME_MAX_LENGTH && name[i_name]; ++i_name)
        pcb->name[i_name] = name[i_name];
    pcb->name[i_name] = '\0';

    push_back_ilist(&processes, &pcb->processes_node);
    
    return pcb;
}

/*
    此函数被调用时一定在用户虚拟地址空间中
    在最低地址的位图中查找一个空闲的用户栈空间
*/
static void *alloc_thread_user_stack(void)
{
    uint32_t *usr_bmps = (uint32_t*)USER_STACK_BITMAP_ADDR;
    for(size_t i = 0;i != USER_THREAD_STACK_BITMAP_COUNT; ++i)
    {
        if(!usr_bmps[i])
            continue;
        uint32_t local_idx = _find_lowest_nonzero_bit(usr_bmps[i]);
        usr_bmps[i] &= ~(1 << local_idx);
        return (void*)((uint32_t)0xc0000000 - 4 -
            (uint32_t)(((i << 5) + local_idx) * (uint32_t)USER_STACK_SIZE));
    }
    return NULL;
}

/* 初始化进程地址空间 */
static void init_process_addr_space(void)
{
    // 填充线程栈位图
    for(size_t i = 0;i != USER_THREAD_STACK_BITMAP_COUNT; ++i)
        ((uint32_t*)USER_STACK_BITMAP_ADDR)[i] = 0xffffffff;
}

/* 进程入口 */
static void process_thread_entry(process_exec_func func,
    uint32_t data_seg_sel, uint32_t code_seg_sel, uint32_t stack_seg_sel)
{
    // 这里直接关中断
    // 等会儿伪装的中断退出函数那里，eflags中IF是打开的
    _disable_intr();

    struct TCB *tcb = get_cur_TCB();
    struct PCB *pcb = tcb->pcb;

    // 初始化进程地址空间
    if(!pcb->addr_space_inited)
    {
        init_process_addr_space();
        pcb->addr_space_inited = true;
    }

    // 填充内核栈最高处的intr_stack，为降低特权级做准备

    tcb->ker_stack += sizeof(struct thread_init_stack);
    struct intr_stack_bak *intr_stack =
        (struct intr_stack_bak*)tcb->ker_stack;
    intr_stack->edi = 0;
    intr_stack->esi = 0;
    intr_stack->ebp = 0;
    intr_stack->esp_dummy = 0;
    intr_stack->ebx = 0;
    intr_stack->edx = 0;
    intr_stack->ecx = 0;
    intr_stack->eax = 0;
    intr_stack->gs = data_seg_sel;
    intr_stack->ds = data_seg_sel;
    intr_stack->es = data_seg_sel;
    intr_stack->fs = data_seg_sel;
    intr_stack->eip = (uint32_t)func;
    intr_stack->cs = code_seg_sel;
    intr_stack->eflags = (1 << 1) | (1 << 9);
    intr_stack->esp = (uint32_t)alloc_thread_user_stack();
    intr_stack->ss = stack_seg_sel;

    extern void intr_proc_end(void); // defined in interrupt.s

    asm volatile ("movl %0, %%esp;"
                  "jmp intr_proc_end"
                  :
                  : "g" (intr_stack)
                  : "memory");
}

static void process_thread_entry_PL_0(process_exec_func func)
{
    process_thread_entry(func,
        SEG_SEL_KERNEL_DATA, SEG_SEL_KERNEL_CODE, SEG_SEL_KERNEL_STACK);
}

static void process_thread_entry_PL_3(process_exec_func func)
{
    process_thread_entry(func,
        SEG_SEL_USER_DATA, SEG_SEL_USER_CODE, SEG_SEL_USER_STACK);
}

/* 把bootloader以来一直在跑的东西封装成进程 */
static void init_bootloader_process(void)
{
    struct PCB *pcb = alloc_PCB();
    struct TCB *tcb = get_cur_TCB();

    strcpy(pcb->name, "kernel process");

    pcb->addr_space = get_ker_vir_addr_space();
    pcb->addr_space_inited = true;

    pcb->pid = 0;
    pcb->is_PL_0 = true;

    init_ilist(&pcb->threads_list);
    push_back_ilist(&pcb->threads_list, &tcb->threads_in_proc_node);
        
    tcb->pcb = pcb;
}

void init_process_man(void)
{
    init_TSS();
    init_user_segments();

    _load_GDT((uint32_t)GDT_START, 8 * 6 - 1);
    _ltr(TSS_SEL);

    init_ilist(&processes);
    init_freelist(&PCB_freelist);

    init_bootloader_process();
}

void set_tss_esp0(uint32_t esp0)
{
    tss.esp0 = esp0;
}

void create_process(const char *name, process_exec_func func, bool is_PL_0)
{
    intr_state intr_s = fetch_and_disable_intr();

    struct PCB *pcb = create_empty_process(name, is_PL_0);

    thread_exec_func thread_entry = (thread_exec_func)(is_PL_0 ? process_thread_entry_PL_0 :
                                                                 process_thread_entry_PL_3);

    struct TCB *tcb = create_thread(thread_entry, func, pcb);
    push_back_ilist(&pcb->threads_list, &tcb->threads_in_proc_node);

    set_intr_state(intr_s);
}
