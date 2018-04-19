#ifndef TINY_OS_SYSMSG_H
#define TINY_OS_SYSMSG_H

#include <lib/bool.h>
#include <lib/intdef.h>

/*
    每个进程都有一个消息队列，位于内核地址空间中，大小非常有限
    进程通过系统调用访问这个队列

    发送方和接收方阻不阻塞在系统调用层面实现，不在消息队列本身层面实现
*/

/*
    内核进程消息类型
    只能是预定义好的，并不能运行时注册增删
*/
typedef uint32_t sysmsg_type;

/* 合法的消息类型 */

#define SYSMSG_TYPE_KEYBOARD 0 /* 键盘按下或释放，参数定义在kernel/kbdriver.h中 */

/*
    内核消息参数字节数
    消息传递并不是拿来大规模发送数据的，所以允许的参数就很有限了……
    至于这块空间如何解释，每种消息有自己的约定
*/
#define SYSMSG_PARAM_SIZE 12

/* 内核进程消息 */
struct sysmsg
{
    sysmsg_type type;
    uint8_t params[SYSMSG_PARAM_SIZE];
};

/* 内核消息队列句柄，放到PCB中 */
struct sysmsg_queue
{
    // 消息缓冲区，逻辑上为环形
    struct sysmsg *msgs;
    // 队列头部下标，msgs[head]为第一个有效消息
    size_t head;
    // 队列尾部下标，msgs[tail]为下一条来的消息的空位
    size_t tail;
    // 队列中的消息数量
    size_t size;    
};

/* 初始化内核进程消息管理系统 */
void init_sysmsg(void);

/*
    初始化一个给定的内核消息队列
    包括分配队列空间
*/
void init_sysmsg_queue(struct sysmsg_queue *queue);

/* 干掉一个内核消息队列，一般在进程销毁的时候调用 */
void destroy_sysmsg_queue(struct sysmsg_queue *queue);

/*
    发送一条消息到某个队列，返回是否发送成功
    msg中的数据会被copy走
*/
bool send_sysmsg(struct sysmsg_queue *queue, const struct sysmsg *msg);

/* 一个消息队列是否为空 */
bool is_sysmsg_queue_empty(const struct sysmsg_queue *queue);

/* 一个消息队列是否已满 */
bool is_sysmsg_queue_full(const struct sysmsg_queue *queue);

/*
    取得消息队列第一条消息，并不弹出
    若队列为空，返回NULL
*/
const struct sysmsg *fetch_sysmsg_queue_front(const struct sysmsg_queue *queue);

/*
    弹出队列第一条消息
    若队列为空，返回false
*/
bool pop_sysmsg_queue_front(struct sysmsg_queue *queue, struct sysmsg *output);

#endif /* TINY_OS_SYSMSG_H */
