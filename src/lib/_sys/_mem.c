#include <shared/asm.h>
#include <shared/proc_mem.h>
#include <shared/string.h>
#include <shared/utility.h>

#include <lib/_sys/_mem.h>

#define _init_usr_addr_interval \
    ((struct usr_addr_interval *)USR_ADDR_INTERVAL_ADDR)

typedef uint32_t _mem_node_desc;

struct _mem_used_node
{
    _mem_node_desc desc;
    size_t last_nei;
};

STATIC_ASSERT(sizeof(struct _mem_used_node) == 8,
              invalid_size_of_mem_used_node);

struct _mem_free_node
{
    _mem_node_desc desc;
    size_t last_nei;

    // freelist中的前驱和后继
    size_t last_free;
    size_t next_free;
};

STATIC_ASSERT(sizeof(struct _mem_free_node) == 16,
              invalid_size_of_mem_free_node);

#define IS_MEM_NODE_USED(NODE) (((struct _mem_used_node *)NODE)->desc & 1)
#define IS_MEM_NODE_FREE(NODE) (!IS_MEM_NODE_USED(NODE))

#define SET_MEM_NODE_USED(NODE) \
    do { ((struct _mem_used_node *)NODE)->desc |= 1; } while(0)
#define SET_MEM_NODE_FREE(NODE) \
    do { ((struct _mem_used_node *)NODE)->desc &= ~1; } while(0)

#define GET_MEM_NODE_SIZE(NODE) (((struct _mem_used_node *)NODE)->desc & ~1)
#define SET_MEM_NODE_SIZE(NODE, SIZE) \
    do { \
        ((struct _mem_used_node *)NODE)->desc = \
            (SIZE) | (((struct _mem_used_node *)NODE)->desc & 1); \
    } while(0)

/*
    由于free node是16字节，因此空闲节点分为：
        [2^4, 2^5), [2^5, 2^6), [2^6, 2^7), ..., [2^30, 2^31), [2^31, 2^32)
    共 (31 - 4 + 1) = 28 类
*/
#define FREE_ENTRY_COUNT 28

/* 一个节点最小能有多小 */
#define MIN_NODE_SIZE 16

/*
    空闲区域的自由链表入口
    下标为i的项代表大小在 [2^(i + 4), 2^(i + 5)) 中的节点入口
    某项为空表示没有该区间内的空闲节点
*/
static struct _mem_free_node *free_entrys[FREE_ENTRY_COUNT];

/* 将一个size转为相应的entry index */
#define SIZE_TO_ENTRY_INDEX(SIZE) \
    ((SIZE) < 16 ? 0 : (find_highest_nonzero_bit(SIZE) - 4))

/*
    malloc初始的两个区间
    IMPROVE: 其实这个应该抽象一下，把malloc做成基于可参数化的context的
*/
static size_t beg1, end1, beg2, end2;

/* 将一个freelist中的node从中摘除 */
static void erase_from_freelist(struct _mem_free_node *n)
{
    struct _mem_free_node *last = (struct _mem_free_node *)n->last_free,
                          *next = (struct _mem_free_node *)n->next_free;
    if(last)
        last->next_free = n->next_free;
    if(next)
        next->last_free = n->last_free;
    
    // 如果是freelist头部，需要单独处理
    uint32_t idx = SIZE_TO_ENTRY_INDEX(GET_MEM_NODE_SIZE(n));
    if(free_entrys[idx] == n)
        free_entrys[idx] = (struct _mem_free_node *)n->next_free;
}

static inline uint32_t get_next_nei(void *node)
{
    struct _mem_used_node *n = (struct _mem_used_node *)(node);
    size_t ret = (size_t)n + GET_MEM_NODE_SIZE(n);
    if((end1 <= ret && ret < beg2) || ret >= end2)
        return 0;
    return ret;
}

/* 将一个被占用的节点加入自由链表中 */
static void add_to_freelist(struct _mem_used_node *n)
{
    size_t size = GET_MEM_NODE_SIZE(n);
    if(size < 16)
        return;
    
    // 看看是不是可以和last_nei合并
    if(n->last_nei && IS_MEM_NODE_FREE(n->last_nei))
    {
        struct _mem_free_node *last_nei = (struct _mem_free_node *)n->last_nei;
        erase_from_freelist(last_nei);

        size_t sum_size = GET_MEM_NODE_SIZE(last_nei) + size;

        struct _mem_free_node *next_nei = (struct _mem_free_node *)get_next_nei(n);
        if(next_nei)
            next_nei->last_nei = (uint32_t)last_nei;

        SET_MEM_NODE_SIZE(last_nei, sum_size);
        SET_MEM_NODE_USED(last_nei);

        add_to_freelist((struct _mem_used_node *)last_nei);
        return;
    }

    // 再看看是不是可以和next_nei合并
    struct _mem_free_node *next_nei = (struct _mem_free_node *)get_next_nei(n);
    if(next_nei && IS_MEM_NODE_FREE(next_nei))
    {
        erase_from_freelist(next_nei);
        
        size_t sum_size = size + GET_MEM_NODE_SIZE(next_nei);

        struct _mem_free_node *next_next_nei =
            (struct _mem_free_node *)get_next_nei(next_nei);
        if(next_next_nei)
            next_next_nei->last_nei = (uint32_t)n;

        SET_MEM_NODE_SIZE(n, sum_size);
        SET_MEM_NODE_USED(n);

        add_to_freelist((struct _mem_used_node *)n);
        return;
    }

    // 都不能合并，好吧，直接加入freelist

    size_t idx = SIZE_TO_ENTRY_INDEX(size);
    SET_MEM_NODE_FREE(n);
    struct _mem_free_node *fn = (struct _mem_free_node *)n;
    fn->last_free = 0;
    fn->next_free = (size_t)free_entrys[idx];
    if(fn->next_free)
        ((struct _mem_free_node *)fn->next_free)->last_free = (uint32_t)fn;
    free_entrys[idx] = fn;
}

/* 设[beg, size)是一块从未纳入使用的内存区，试着将它加入该存储管理系统中 */
static void init_free_mem(size_t beg, size_t size)
{
    if(size < 16)
        return;

    // 伪装成used node加入自由链表中

    struct _mem_used_node *node = (struct _mem_used_node *)beg;
    SET_MEM_NODE_SIZE(node, size);
    SET_MEM_NODE_USED(node);
    node->last_nei = 0;

    add_to_freelist(node);
}

/* 申请一个指定大小的节点 */
static struct _mem_used_node *alloc_from_freelist(size_t size)
{
    // 看看应该到哪个freelist中取
    // 若size \in [2^k, 2^(k+1))，则可到[2^(k+1), 2^(k+2))中获取
    // 但若size正好是2^k的形式，就不用委屈自己
    uint32_t ori_idx = SIZE_TO_ENTRY_INDEX(size);

    struct _mem_free_node *fn;

    // 是否优先到更大的区间中去查找空间
    bool idximp = (size > (1 << (ori_idx + 4)));

    uint32_t idx = ori_idx + idximp;
    while(idx < FREE_ENTRY_COUNT && !free_entrys[idx])
        ++idx;

    // 如果没有区间有空闲了，且之前优先找了更大的区间，尝试在小一级区间中查找
    if(idx >= FREE_ENTRY_COUNT)
    {
        if(!idximp)
            return NULL;

        fn = free_entrys[ori_idx];
        while(fn && GET_MEM_NODE_SIZE(fn) < size)
            fn = (struct _mem_free_node *)fn->next_free;

        if(!fn)
            return NULL;
    }
    else
        fn = free_entrys[idx];
    
    
    // 从对应freelist中摘掉头部
    erase_from_freelist(fn);
    
    // 注意这里要先把fn标记为已使用，再试图把fn中多余的空间加入freelist
    // 否则fn标记为未用，这时将多余空间加入freelist时又会和fn合并上，导致GG
    SET_MEM_NODE_USED(fn);

    // 看看取走size之后剩下的空间是否可堪一用
    size_t remain_size = GET_MEM_NODE_SIZE(fn) - size;
    if(remain_size >= MIN_NODE_SIZE)
    {
        SET_MEM_NODE_SIZE(fn, size);

        struct _mem_used_node *nn =
            (struct _mem_used_node *)((uint32_t)fn + size);

        SET_MEM_NODE_SIZE(nn, remain_size);
        SET_MEM_NODE_USED(nn);
        nn->last_nei = (uint32_t)fn;

        add_to_freelist(nn);
    }

    return (struct _mem_used_node *)fn;
}

void _init_mem_man()
{
    memset((char*)free_entrys, 0x0, sizeof(free_entrys));

    _init_usr_addr_interval->size1 &= ~3;
    _init_usr_addr_interval->size2 &= ~3;
    
    // 将两个初始空闲区加入自由链表

    beg1 = _init_usr_addr_interval->beg1;
    end1 = beg1 + _init_usr_addr_interval->size1;
    
    beg2 = _init_usr_addr_interval->beg2;
    end2 = beg2 + _init_usr_addr_interval->size2;

    init_free_mem(_init_usr_addr_interval->beg1,
                  _init_usr_addr_interval->size1);
    
    init_free_mem(_init_usr_addr_interval->beg2,
                  _init_usr_addr_interval->size2);
}

void *_malloc(size_t size)
{
    // 不能太小，加上used node size后至少要和MIN_NODE_SIZE一样大
    size = MAX(size, MIN_NODE_SIZE - sizeof(struct _mem_used_node));

    // 加上前缀的size，并保证是四字节的整数倍
    size_t node_size = ((size + sizeof(struct _mem_used_node)) + 3) & (~3);

    struct _mem_used_node *node = alloc_from_freelist(node_size);
    return node ? (void*)((uint32_t)node + sizeof(struct _mem_used_node)) : NULL; 
}

void _free(void *ptr)
{
    if(ptr)
    {
        add_to_freelist((struct _mem_used_node *)
            ((uint32_t)ptr - sizeof(struct _mem_used_node)));
    }
}
