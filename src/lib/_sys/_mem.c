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

/*
    空闲区域的自由链表入口
    下标为i的项代表大小在 [2^(i + 4), 2^(i + 5)) 中的节点入口
    某项为空表示没有该区间内的空闲节点
*/
static struct _mem_free_node *free_entrys[28];

static size_t beg1, end1, beg2, end2;

static inline uint32_t get_next_nei(void *node)
{
    struct _mem_used_node *n = (struct _mem_used_node)(node);
    size_t ret = (size_t)n + GET_MEM_NODE_SIZE(n);
    return (ret < end1 || (beg2 <= ret && ret < end2)) ? ret : 0;
}

static void add_to_freelist(struct _mem_used_node *n)
{
    size_t size = GET_MEM_NODE_SIZE(n);
    if(size < 16)
        return;
    
    // 计算节点在free_entrys中的位置
    size_t idx = find_highest_nonzero_bit(size) - 4;
    
    // 看看是不是可以和last_nei合并
    if(n->last_nei && IS_MEM_NODE_FREE(n->last_nei))
    {
        // TODO
    }

    // 再看看是不是可以和next_nei合并
    size_t next_nei = get_next_nei(n);
    if(next_nei && IS_MEM_NODE_FREE(next_nei))
    {
        // TODO
    }

    // 都不能合并，好吧，直接加入freelist

    // TODO
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

void _init_mem_man()
{
    memset((char*)free_entrys, 0x0, sizeof(free_entrys));
    
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
