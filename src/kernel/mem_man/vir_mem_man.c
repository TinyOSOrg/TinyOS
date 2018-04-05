#include <kernel/asm.h>
#include <kernel/boot.h>
#include <kernel/intr_entry.h>
#include <kernel/page_desc.h>
#include <kernel/phy_mem_man.h>
#include <kernel/vir_mem_man.h>

#include <lib/string.h>

#include <common.h>

/*
    虚拟页目录的空间一开始就要在内核地址空间中准备好
*/

struct PDE_struct
{
    uint32_t PTE[1024];
};

struct PTE_struct
{
    uint32_t page[1024];
};

static struct PDE_struct *ker_vir_addr_space;

static struct PDE_struct *cur_vir_addr_space;

static uint32_t ker_PDE_phy;

static uint32_t cur_PDE_phy;

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

/* 缺页中断处理 */
void page_fault_handler(void)
{
    // 1. 取得是哪个页缺失了
    // 2. 看看页表是否缺失，缺了的话，给它分配一个物理页
    // 3. 给缺失的页面分配一个物理页，写到对应页表里
    // 4. 刷新TLB条目

    // 取得cr2寄存器内容，即引发pagefault的虚拟地址
    uint32_t cr2 = _get_cr2();

    // 计算页表索引和物理页索引
    uint32_t PTEidx = cr2 >> 22;
    uint32_t page_idx = (cr2 >> 12) & 0x3ff;

    // 若页表不存在，就分配一个
    struct PTE_struct *PTEaddr = (struct PTE_struct*)((0x3ff << 22) | (PTEidx << 12));
    if(!(cur_vir_addr_space->PTE[PTEidx] & PAGE_PRESENT_TRUE))
    {
        cur_vir_addr_space->PTE[PTEidx] = iPDE(
                        alloc_phy_page(true),
                        PAGE_PRESENT_TRUE,
                        PAGE_RW_READ_WRITE,
                        PAGE_USER_USER);
        
        // 清空新的页表
        memset((char*)PTEaddr, 0, 4096);
    }

    // 给缺失页面分配一个物理页
    // 缺页中断分配的结果都是非常驻内存的，常驻内存的页必须是内核预先分配的页
    PTEaddr->page[page_idx] = iPTE(
                    alloc_phy_page(false),
                    PAGE_PRESENT_TRUE,
                    PAGE_RW_READ_WRITE,
                    PAGE_USER_USER);
    
    // 刷新TLB
    _refresh_vir_addr_in_TLB(cr2);
}

void init_vir_mem_man(void)
{
    ker_vir_addr_space = (struct PDE_struct*)KER_PDE_VIR_ADDR;
    cur_vir_addr_space = ker_vir_addr_space;

    ker_PDE_phy = KER_PDE_PHY_ADDR;
    cur_PDE_phy = ker_PDE_phy;

    intr_function[INTR_NUMBER_PAGE_FAULT] = page_fault_handler;
}
