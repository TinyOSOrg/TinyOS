#ifndef TINY_OS_SYSMSG_SRC_H
#define TINY_OS_SYSMSG_SRC_H

#include <shared/ptrlist.h>

/*
    因为效率问题，系统中消息总线是不打算做的
    所以每种消息源维护自己要通知的进程列表，本文件中的接口提供了该机制
    反过来，当进程挂掉的时候，也要通知到消息源

    于是每个消息源维护一个链表A，数据是进程PCB
    同时进程也维护一个链表B，遍历B可以从所有和该进程相关的A中把该进程删掉
*/

struct PCB;

/* 消息源持有的接收者列表，即A链表 */
struct sysmsg_receiver_list
{
    ilist processes;
};

/* PCB端的链表B的节点 */
struct sysmsg_rcv_src_list_node
{
    struct PCB *pcb;
    struct ilist_node rcv_node; // A中的节点，即消息源持有的接收者链表中的节点
    struct ilist_node src_node; // B中的节点，即PCB持有的消息源链表中的节点
};

/* PCB端的消息源列表，即B链表 */
struct sysmsg_source_list
{
    ilist sources;
};

/* 两种表的初始化和销毁 */

#define init_sysmsg_receiver_list(X) init_ilist(&(X)->processes)

#define init_sysmsg_sources_list(X)  init_ilist(&(X)->sources)

void destroy_sysmsg_receiver_list(struct sysmsg_receiver_list *receiver_list);

void destroy_sysmsg_source_list(struct sysmsg_source_list *source_list);

/* 把进程注册到消息源，提供A、B链表即可，会建立起其间的链接 */
void register_sysmsg_source(struct PCB *pcb,
                            struct sysmsg_receiver_list *rcv,
                            struct sysmsg_source_list   *src);

/* 没错，注册了就别想删（因为解除一个链接要线性搜索链表…… */

#endif /* TINY_OS_SYSMSG_SRC_H */
