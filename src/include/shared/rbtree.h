#ifndef TINY_OS_SHARED_RBTREE_H
#define TINY_OS_SHARED_RBTREE_H

#include <shared/bool.h>
#include <shared/intdef.h>

/*
    侵入式红黑树节点
    要求：任何节点地址都是2字节对齐的，否则UB，虽然这很难有机会违背……
*/
struct rb_node
{
    // parent的最低位记录颜色
    struct rb_node *parent;
    struct rb_node *left, *right;
};

/* 红黑树句柄 */
struct rb_tree
{
    struct rb_node *root;
    struct rb_node nil;
};

void rb_init(struct rb_tree *tree);

/* 用于比较红黑树中两个节点的键值大小 */
typedef bool (*rb_less_func)(const void *L, const void *R);

/* 在红黑树中查找指定键值，找不到时返回NULL
    对每个查询路径上的节点地址N，less被以下面的方式调用：
        less((char*)N + key_offset, key) 或
        less(key, (char*)N + key_offset)
    key_offset可以用下面定义的宏取得
*/
struct rb_node *rb_find(struct rb_tree *tree, int32_t key_offset,
                        const void *key, rb_less_func less);

/*
    插入节点
    若节点已经存在，返回false
*/
bool rb_insert(struct rb_tree *tree, struct rb_node *node,
               int32_t key_offset, rb_less_func less);

void rb_erase(struct rb_tree *tree, struct rb_node *node,
              int32_t key_offset, rb_less_func less);

struct rb_node *rb_minimum(struct rb_node *nil, struct rb_node *node);

/*
    给定一个结构名和一个成员名，取得 -(成员在结构体内的偏移量)
    在成员指针上加上这一偏移量，就能得到结构体指针
*/
#define RB_MEM_TO_STRUCT_OFFSET(STRUCT, MEM) \
    (-(int32_t)(&((STRUCT*)0->MEM)))

/*
    给定一个结构体名和其中的两个成员名，取得 成员2偏移量 - 成员1偏移量
    在成员1指针上加上该偏移，就能得到成员2指针
*/
#define RB_MEM_TO_MEM_OFFSET(STRUCT, MEM1, MEM2) \
    ((int32_t)(&(((STRUCT*)0)->MEM2)) - \
     (int32_t)(&(((STRUCT*)0)->MEM1)))

#endif /* TINY_OS_SHARED_RBTREE_H */
