#ifndef TINY_OS_RLIST_NODE_ALLOC_H
#define TINY_OS_RLIST_NODE_ALLOC_H

#include <shared/ptrlist.h>

/*
    内核中许多地方要用到rlist
    这里做一个公共的节点分配和释放器
    谁让内核层面没有malloc和free可以用呢
*/

/* 在虚拟内存管理系统初始化之后、用到rlist node allocator之前调用 */
void init_kernel_rlist_node_alloc();

/* 申请一个常驻内存的rlist node */
struct rlist_node *kernel_resident_rlist_node_alloc();

/* 释放一个常驻内存的rlist node */
void kernel_resident_rlist_node_dealloc(struct rlist_node *node);

#endif /* TINY_OS_RLIST_NODE_ALLOC_H */
