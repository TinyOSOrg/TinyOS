#ifndef TINY_OS_MEM_MAN_H
#define TINY_OS_MEM_MAN_H

/*
    页级存储管理
*/

/*
    物理页管理：
    设共有N个物理页
    32-bit位图管理32个4K页面是否被占用，共N/32个32-bit位图，称为L3位图
    每两个32-bit位图由一位来标志其是否被完全占用，构成32-bit位图，共需N/32^2个，称为L2位图
    以此类推，定义出L1位图和L0位图来。即使是4GB的内存也就32个L1位图，一个L0位图。所以四级的位图方案绝对够用了。
*/

#include <lib/bool.h>
#include <lib/intdef.h>

struct mem_page_pool
{
    //mem_page_pool是个变长结构体，所以大小记一下
    size_t struct_size;

    /*
        应满足begin < end，begin和end值的单位为4K
        该内存池的范围为[begin, end)
        pool_pages = end - begin
    */
    size_t begin, end, pool_pages;

    // 一个L0位图就足以覆盖4GB内存
    // 1表示尚可用，0表示已被占用
    uint32_t bitmap_L0;
    
    /*
        应满足：
            bitmap_count_L1 = ceil(bitmap_count_L2 / 32)
            bitmap_count_L2 = ceil(bitmap_count_L3 / 32)
            bitmap_count_L3 = ceil((end - begin) / 32)
    */
    size_t bitmap_count_L1;
    size_t bitmap_count_L2;
    size_t bitmap_count_L3;
    
    uint32_t *bitmap_L1;
    uint32_t *bitmap_L2;
    uint32_t *bitmap_L3;

    // 页面是否应常驻内存所构成的位图
    // 1表示常驻，0表示可换出
    uint32_t *bitmap_resident;
    
    // gcc扩展：零长数组
    uint32_t bitmap_data[0];
};

void init_mem_man(void);

size_t get_mem_total_bytes(void);

// 分配一块固定大小的、在整个系统运行期间常驻的内核内存区域
void *alloc_static_kernel_mem(size_t bytes, size_t align_bytes);

#endif // TINY_OS_MEM_MAN_H
