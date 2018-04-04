#ifndef TINY_OS_PHY_MEM_MAN_H
#define TINY_OS_PHY_MEM_MAN_H

/* 页级存储管理 */

#include <lib/bool.h>
#include <lib/intdef.h>

size_t get_mem_total_bytes(void);

/* 分配一块固定大小的、在整个系统运行期间常驻的内核内存区域 */
void *alloc_static_kernel_mem(size_t bytes, size_t align_bytes);

/* 初始化整个页内存管理器 */
void init_mem_man(void);

/*
    分配一个物理页，返回值为物理地址
    resident为false表示非常驻，反之表示常驻内存（不会被换出）
*/
uint32_t alloc_phy_page(bool resident);

/*
    释放一个物理页
    若页面不在物理页管理器管理范围内，或本来就没被占用，则返回false
*/
void free_phy_page(uint32_t page_phy_addr);

/* 还剩多少个物理页空闲 */
uint32_t get_free_phy_page_count(void);

#endif /* TINY_OS_PHY_MEM_MAN_H */
