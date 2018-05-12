#ifndef TINY_OS_VIR_MEM_MAN_H
#define TINY_OS_VIR_MEM_MAN_H

/* 虚拟地址空间的创建、启用和销毁 */

#include <shared/bool.h>
#include <shared/intdef.h>

/* 最多支持多少个虚拟地址空间（除ker_addr_space） */
#define MAX_VIR_ADDR_SPACE_COUNT 64

/* 初始化整个虚拟地址空间管理系统，在init_phy_mem_man()之后即可调用 */
void init_vir_mem_man();

/* 分配一个内核页（占用内核虚拟地址空间） */
void *alloc_ker_page(bool resident);

/* 释放一个内核页 */
void free_ker_page(void *page);

/*
    虚拟地址空间句柄
*/
typedef struct _vir_addr_space vir_addr_space;

/*
    创建一个虚拟地址空间并返回其句柄
    初始虚拟地址空间中有些部分是已经被用了的，使用情况如下：
        高1GB - 4KB永远和内核共享
        最高的页目录项指向页目录自身所在物理页（虽然按理说它应该指向一个页表）

    创建的虚拟页目录本身所在的物理页是常驻内存的
*/
vir_addr_space *create_vir_addr_space();

/*
    将某个虚拟地址空间设置为当前正使用
    不合法参数将导致未定义行为
*/
void set_current_vir_addr_space(vir_addr_space *addr_space);

/* 取得当前虚拟地址空间句柄 */
vir_addr_space *get_current_vir_addr_space();

/*
    取得自bootloader开始内核所使用的虚拟地址空间句柄
    虽然这玩意儿并没有什么卵用
*/
vir_addr_space *get_ker_vir_addr_space();

/*
    销毁某个虚拟地址空间
    其占用的所有物理页均将被释放
*/
void destroy_vir_addr_space(vir_addr_space *addr_space);

/* 返回一个虚拟空间标号 */
size_t get_usr_vir_addr_idx(vir_addr_space *rec);

#endif /* TINY_OS_VIR_MEM_MAN_H */
