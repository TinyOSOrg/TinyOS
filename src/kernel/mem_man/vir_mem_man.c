#include <kernel/asm.h>
#include <kernel/page_desc.h>
#include <kernel/phy_mem_man.h>
#include <kernel/vir_mem_man.h>

#include <common.h>

/* 虚拟地址空间管理设计
    虚拟页目录的空间一开始就要在内核地址空间中准备好
    记录一下哪些是用了的，哪些是没用的
*/

/* 内核虚拟地址空间 */
//static vir_addr_space ker_vir_addr_space;

/* 当前虚拟地址空间 */
static vir_addr_space cur_vir_addr_space;

/* 合成一个页目录项 */
static inline uint32_t iPDE(uint32_t PTE_phy_addr,
                            page_desc_attrib present,
                            page_desc_attrib read_write,
                            page_desc_attrib user)
{
    return PTE_phy_addr | present | read_write | user;
}

/* 合成一个页表项 */
static inline uint32_t iPTE(uint32_t page_phy_addr,
                            page_desc_attrib present,
                            page_desc_attrib read_write,
                            page_desc_attrib user)
{
    return page_phy_addr | present | read_write | user;
}

/* （在当前虚拟地址空间内）虚拟地址转物理地址 */
static inline uint32_t vir_to_phy(uint32_t vir)
{
    return *(uint32_t*)((0x3ff << 22) |
                        ((vir >> 22) << 12) |
                        ((vir >> 10) & 0xffc))
          + (vir & 0xfff);
}

void refresh_TLB(void)
{
    _load_cr3(cur_vir_addr_space);
}
