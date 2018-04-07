#include <kernel/asm.h>
#include <kernel/assert.h>
#include <kernel/boot.h>
#include <kernel/intr_entry.h>
#include <kernel/print.h>

#include <kernel/mem_man/page_desc.h>
#include <kernel/mem_man/phy_mem_man.h>
#include <kernel/mem_man/vir_mem_man.h>
#include <kernel/mem_man/vir_page_man.h>

#include <lib/string.h>

#include <common.h>

struct PDE_struct
{
    uint32_t PTE[1024];
};

struct PTE_struct
{
    uint32_t page[1024];
};

/*
    虚拟地址空间记录
    完整地描述了一个用户虚拟地址空间
*/
struct _vir_addr_space
{
    struct PDE_struct *vir_PDE;
    uint32_t phy_PDE;
};

/*
    以自由链表管理用户虚拟地址空间记录
    sizeof(struct _vir_addr_space_freelist_node)
        <= sizeof(struct _vir_addr_space)
*/
struct _vir_addr_space_freelist_node
{
    struct _vir_addr_space_freelist_node *last;
    struct _vir_addr_space_freelist_node *next;
};

/* 最多支持多少个虚拟地址空间（除ker_addr_space） */
#define MAX_VIR_ADDR_SPACE_COUNT 64

/* 指向用户虚拟地址空间记录数组 */
vir_addr_space *usr_addr_spaces_arr;

/*
    用户虚拟地址空间记录的自由链表的首个节点
    为空说明自由链表已空
*/
struct _vir_addr_space_freelist_node *empty_usr_addr_space_rec;

/*
    预先分配的用户虚拟页目录数组
    其访问下标和usr_addr_spaces_arr对应
*/
struct PDE_struct **usr_PDEs;

/* 自boot其就在用的内核页表记录 */
static vir_addr_space ker_addr_space;

/* 当前页表记录 */
static vir_addr_space *cur_addr_space;

/* 内核页级虚存分配器 */
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

/* 取得当前虚拟地址空间的第PTE_idx个页表的虚拟地址 */
static inline struct PTE_struct *make_PTE_vir_addr(size_t PTE_idx)
{
    return (struct PTE_struct*)((0x3ff << 22) | (PTE_idx << 12));
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
    uint32_t PTE_idx = cr2 >> 22;
    uint32_t page_idx = (cr2 >> 12) & 0x3ff;

    // 若页表不存在，就分配一个
    struct PTE_struct *PTEaddr = make_PTE_vir_addr(PTE_idx);
    if(!(cur_addr_space->vir_PDE->PTE[PTE_idx] & PAGE_PRESENT_TRUE))
    {
        cur_addr_space->vir_PDE->PTE[PTE_idx] = iPDE(
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

/* 初始化内核虚拟页管理器 */
static void init_ker_page_man(void)
{
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

/*
    给定一个用户虚拟地址空间记录，返回其唯一的下标
    虚拟地址空间记录的下标和对应虚拟页下标是相同的
*/
static size_t get_usr_vir_addr_idx(void *rec)
{
    return ((char*)rec - (char*)usr_addr_spaces_arr) / sizeof(vir_addr_space);
}

/* 在空闲的用户虚拟地址空间记录自由链表中插入一个元素 */
static void insert_to_empty_usr_addr_space_rec(void *rec)
{
    struct _vir_addr_space_freelist_node *new_node =
        (struct _vir_addr_space_freelist_node*)rec;
    new_node->last = NULL;
    new_node->next = empty_usr_addr_space_rec;
    
    if(!empty_usr_addr_space_rec)
        empty_usr_addr_space_rec = new_node;
    else
        empty_usr_addr_space_rec->last = new_node;

    empty_usr_addr_space_rec = new_node;
}

/*
    在空闲的用户虚拟地址空间记录自由链表中取出一个元素
    若记录已满，返回NULL
*/
static vir_addr_space *new_empty_usr_addr_space(void)
{
    if(!empty_usr_addr_space_rec)
        return NULL;

    if(empty_usr_addr_space_rec->next)
        empty_usr_addr_space_rec->next->last = NULL;
    
    vir_addr_space *rt = (vir_addr_space*)empty_usr_addr_space_rec;
    empty_usr_addr_space_rec = empty_usr_addr_space_rec->next;

    return rt;
}

/* 初始化用户虚拟地址空间管理系统 */
static void init_usr_vir_addr_space_man(void)
{
    // 获取记录首元素地址
    usr_addr_spaces_arr = alloc_static_kernel_mem(
            sizeof(vir_addr_space) * MAX_VIR_ADDR_SPACE_COUNT, 4);
    
    // 构造自由链表
    empty_usr_addr_space_rec = NULL;
    for(size_t i = 0;i != MAX_VIR_ADDR_SPACE_COUNT; ++i)
        insert_to_empty_usr_addr_space_rec(&usr_addr_spaces_arr[i]);
    
    // 用户虚拟页空间分配
    usr_PDEs = alloc_static_kernel_mem(
        sizeof(struct PDE_struct*) * MAX_VIR_ADDR_SPACE_COUNT, 4);
    for(size_t i = 0;i != MAX_VIR_ADDR_SPACE_COUNT; ++i)
        usr_PDEs[i] = alloc_ker_page(true);
}

/* 给定一个用户地址空间记录，返回其页目录的虚拟地址 */
static struct PDE_struct *get_page_of_usr_addr_space(vir_addr_space *addr_space)
{
    return usr_PDEs[get_usr_vir_addr_idx(addr_space)];
}

void init_vir_mem_man(void)
{
    // 内核页目录初始化
    ker_addr_space.vir_PDE = (struct PDE_struct*)KER_PDE_VIR_ADDR;
    ker_addr_space.phy_PDE = KER_PDE_PHY_ADDR;
    cur_addr_space = &ker_addr_space;

    // 缺页中断初始化
    set_intr_function(INTR_NUMBER_PAGE_FAULT, page_fault_handler);

    // 内核虚拟页管理器初始化
    init_ker_page_man();

    // 初始化用户虚拟地址空间管理系统
    init_usr_vir_addr_space_man();
}

void *alloc_ker_page(bool resident)
{
    // 取得一个物理页
    uint32_t phy_page = alloc_phy_page(resident);

    // 取得一个虚拟页
    uint32_t vir_page = alloc_page_in_vir_addr_space(ker_page_man);

    // 将虚拟页和物理页关联起来

    // 计算页表索引和页索引
    uint32_t PTE_idx = vir_page >> 22;
    uint32_t page_idx = (vir_page >> 12) & 0x3ff;

    // 页表一定存在，因为高255个页目录项在bootloader那里就填上了
    struct PTE_struct *PTEaddr = make_PTE_vir_addr(PTE_idx);
    ASSERT_S(cur_addr_space->vir_PDE->PTE[PTE_idx] & PAGE_PRESENT_TRUE);

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
    uint32_t PTE_idx = vir_page >> 22;
    uint32_t page_idx = (vir_page >> 12) & 0x3ff;

    struct PTE_struct *PTEaddr = make_PTE_vir_addr(PTE_idx);
    PTEaddr->page[page_idx] = 0;

    _refresh_vir_addr_in_TLB(vir_page);
    
    free_page_in_vir_addr_space(ker_page_man, vir_page);
    free_phy_page(phy_page);
}

vir_addr_space *create_vir_addr_space(void)
{
    // 分配一个虚拟地址空间记录
    vir_addr_space *rec = new_empty_usr_addr_space();
    if(!rec)
        return NULL;
    
    // 取得对应的页目录虚拟地址
    rec->vir_PDE = get_page_of_usr_addr_space(rec);
    // 取得对应的页目录物理地址
    rec->phy_PDE = vir_to_phy((uint32_t)rec->vir_PDE);

    // 清空没用到的页目录
    for(size_t i = 0;i != 768; ++i)
        rec->vir_PDE->PTE[i] = 0;

    // 768-1022项与内核共享
    for(size_t i = 768; i != 1022; ++i)
        rec->vir_PDE->PTE[i] = ker_addr_space.vir_PDE->PTE[i];
    
    // 最后一项指向自身所在页
    rec->vir_PDE->PTE[1023] = iPDE(rec->phy_PDE,
                                   PAGE_PRESENT_TRUE,
                                   PAGE_RW_READ_WRITE,
                                   PAGE_USER_USER);
    
    return rec;
}

void set_current_vir_addr_space(vir_addr_space *addr_space)
{
    cur_addr_space = addr_space;
    _load_cr3(addr_space->phy_PDE);
}

vir_addr_space *get_current_vir_addr_space(void)
{
    return cur_addr_space;
}

vir_addr_space *get_ker_vir_addr_space(void)
{
    return &ker_addr_space;
}

void destroy_vir_addr_space(vir_addr_space *addr_space)
{
    vir_addr_space *old = get_current_vir_addr_space();
    set_current_vir_addr_space(addr_space);

    // 释放其所占用的每个物理页以及页表本身所在物理页（和内核共享的部分除外）
    for(size_t i = 0;i != 768; ++i)
    {
        if(!(addr_space->vir_PDE->PTE[i] & PAGE_PRESENT_TRUE))
            continue;
        
        struct PTE_struct *PTE = make_PTE_vir_addr(i);
        
        // 对页表中的每一项，free_phy_page之
        for(size_t j = 0;j != 1024; ++j)
        {
            if(PTE->page[j] & PAGE_PRESENT_TRUE)
                free_phy_page(PTE->page[j] & ~0xfff);
        }

        free_phy_page(addr_space->vir_PDE->PTE[i] & ~0xfff);
    }

    set_current_vir_addr_space(old);

    // 把addr_space记录插回自由链表
    insert_to_empty_usr_addr_space_rec(addr_space);
}
