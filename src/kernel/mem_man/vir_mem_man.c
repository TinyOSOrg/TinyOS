#include <kernel/asm.h>
#include <kernel/assert.h>
#include <kernel/boot.h>
#include <kernel/intr_entry.h>
#include <kernel/page_desc.h>
#include <kernel/phy_mem_man.h>
#include <kernel/vir_mem_man.h>
#include <kernel/vir_page_man.h>

#include <kernel/print.h>

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

struct _vir_addr_space
{
    struct PDE_struct *vir_PDE;
    uint32_t phy_PDE;
};

/* 自boot其就在用的内核页表记录 */
static vir_addr_space ker_addr_space;

/* 当前页表记录 */
static vir_addr_space *cur_addr_space;

/* 内核虚拟空间管理器 */
static struct vir_page_man *ker_page_man;

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
    return (*(uint32_t*)((0x3ff << 22) |
                        ((vir >> 22) << 12) |
                        ((vir >> 10) & 0xffc)) & ~0xfff)
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
    if(!(cur_addr_space->vir_PDE->PTE[PTEidx] & PAGE_PRESENT_TRUE))
    {
        cur_addr_space->vir_PDE->PTE[PTEidx] = iPDE(
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
    // 内核页目录初始化
    ker_addr_space.vir_PDE = (struct PDE_struct*)KER_PDE_VIR_ADDR;
    ker_addr_space.phy_PDE = KER_PDE_PHY_ADDR;
    cur_addr_space = &ker_addr_space;

    // 缺页中断初始化
    intr_function[INTR_NUMBER_PAGE_FAULT] = page_fault_handler;

    // 内核虚拟页管理器初始化

    // 内核虚拟空间中，截止至静态内核存储区域都已经用了，后面的一直到4GB - 4KB都还空着
    size_t ker_page_begin = (STATIC_KERNEL_MEM_START +
                             STATIC_KERNEL_MEM_SIZE) / 4096;
    size_t ker_page_end   = 0xffffffff / 4096 - 1;
    size_t page_man_byte_size =
        get_vir_page_man_byte_size(ker_page_begin, ker_page_end);

    ker_page_man = alloc_static_kernel_mem(page_man_byte_size,
                                           VIRTUAL_PAGE_MANAGER_ALIGN);
    init_vir_page_man(ker_page_man, ker_page_begin, ker_page_end);
}

void *alloc_ker_page(bool resident)
{
    // 取得一个物理页
    uint32_t phy_page = alloc_phy_page(resident);

    // 取得一个虚拟页
    uint32_t vir_page = alloc_page_in_vir_addr_space(ker_page_man);

    // 将虚拟页和物理页关联起来

    // 计算页表索引和页索引
    uint32_t PTEidx = vir_page >> 22;
    uint32_t page_idx = (vir_page >> 12) & 0x3ff;

    // 页表一定存在，因为高255个页目录项在bootloader那里就填上了
    struct PTE_struct *PTEaddr = (struct PTE_struct*)((0x3ff << 22) | (PTEidx << 12));
    ASSERT_S(cur_addr_space->vir_PDE->PTE[PTEidx] & PAGE_PRESENT_TRUE);

    // 把物理页填入页表
    PTEaddr->page[page_idx] = iPTE(
                phy_page,
                PAGE_PRESENT_TRUE,
                PAGE_RW_READ_WRITE,
                PAGE_USER_USER);

    // 刷新TLB
    _refresh_vir_addr_in_TLB(vir_page);

    return (void*)vir_page;
}

void free_ker_page(void *page)
{
    uint32_t vir_page = (uint32_t)page;
    uint32_t phy_page = vir_to_phy(vir_page);

    // 计算页表索引和页索引
    uint32_t PTEidx = vir_page >> 22;
    uint32_t page_idx = (vir_page >> 12) & 0x3ff;

    struct PTE_struct *PTEaddr = (struct PTE_struct*)((0x3ff << 22) | (PTEidx << 12));
    PTEaddr->page[page_idx] = 0;

    _refresh_vir_addr_in_TLB(vir_page);
    
    free_page_in_vir_addr_space(ker_page_man, vir_page);
    free_phy_page(phy_page);
}
