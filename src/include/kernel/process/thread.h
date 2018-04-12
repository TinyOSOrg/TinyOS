#ifndef TINY_OS_THREAD_H
#define TINY_OS_THREAD_H

#include <lib/intdef.h>

/* 前向声明：process control block */
struct PCB;

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

    // 初始内核栈
    void *init_ker_stack;

    enum thread_state state;

    struct PCB *pcb;
};

/*
    用来创建新内核线程时的函数入口
    void*用于参数传递
*/
typedef void (*thread_exec_func)(void*);

/*
    线程栈初始化时栈顶的内容
    这个结构参考了真像还原一书
    利用ret指令及其参数在栈中分布的特点来实现控制流跨线程跳转
    可以统一第一次进入线程以及以后调度该线程时的操作
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

/* 初始化内核线程管理系统 */
void init_thread_man(void);

/* 创建新内核线程并加入调度 */
struct TCB *create_thread(thread_exec_func func, void *params,
                          struct PCB *pcb);

/* 取得当前线程的TCB */
struct TCB *get_cur_TCB(void);

/* 阻塞自己，注意系统不会维护阻塞线程 */
void block_cur_thread(void);

/* 唤醒一个blocked线程，将其变为ready */
void awake_thread(struct TCB *tcb);

#endif /* TINY_OS_THREAD_H */
