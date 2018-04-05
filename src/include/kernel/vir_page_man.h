#ifndef TINY_OS_VIR_PAGE_MAN_H
#define TINY_OS_VIR_PAGE_MAN_H

#include <lib/intdef.h>

/* 页级虚拟地址空间管理 */
struct vir_page_man;

/* 虚拟页管理器为4字节对齐 */
#define VIRTUAL_PAGE_MANAGER_ALIGN 4

/*
    计算一个虚拟页管理器占多少个字节，以方便空间申请
    管辖的范围是[begin, end)
*/
size_t get_vir_page_man_byte_size(size_t pool_begin, size_t pool_end);

/* 初始化一个虚拟页管理器，要求page_man空间已分配 */
void init_vir_page_man(struct vir_page_man *page_man,
                       size_t pool_begin, size_t pool_end);

/*
    在虚拟地址空间管理器中分配一个页
    虚拟地址空间不足时返回0
*/
uint32_t alloc_page_in_vir_addr_space(struct vir_page_man *page_man);

/*
    在虚拟地址空间中释放一个页
    若addr并非page_man中的已分配页，将导致未定义行为
*/
void free_page_in_vir_addr_space(struct vir_page_man *page_man, uint32_t addr);

#endif /* TINY_OS_VIR_PAGE_MAN_H */
