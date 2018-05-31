#include <kernel/asm.h>
#include <kernel/assert.h>
#include <kernel/boot.h>
#include <kernel/memory/phy_mem_man.h>

#include <shared/utility.h>

/*
    物理页管理：
    设共有N个物理页
    32-bit位图管理32个4K页面是否被占用，共N/32个32-bit位图，称为L3位图
    每两个32-bit位图由一位来标志其是否被完全占用，构成32-bit位图，共需N/32^2个，称为L2位图
    以此类推，定义出L1位图和L0位图来。即使是4GB的内存也就32个L1位图，一个L0位图。所以四级的位图方案绝对够用了。
*/

struct mem_page_pool
{
    // mem_page_pool是个变长结构体，所以大小记一下
    size_t struct_size;

    // 应满足begin < end，begin和end值的单位为4K
    // 该内存池的范围为[begin, end)
    // pool_pages = end - begin
    size_t begin, end;
    size_t pool_pages, unused_pages; //总页数和当前空余页数

    // 一个L0位图就足以覆盖4GB内存
    // 1表示尚可用，0表示已被占用
    uint32_t bitmap_L0;

    // 应满足：
    //    bitmap_count_L1 = ceil(bitmap_count_L2 / 32)
    //    bitmap_count_L2 = ceil(bitmap_count_L3 / 32)
    //    bitmap_count_L3 = ceil((end - begin) / 32)
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

static size_t total_mem_bytes;

/* 为alloc_static_kernel_mem所用，是静态内核存储区域已经使用部分的顶部 */
static void *static_kernel_mem_top;

/* 全局物理内存池 */
static struct mem_page_pool *phy_mem_page_pool;

size_t get_mem_total_bytes()
{
    return total_mem_bytes;
}

void *alloc_static_kernel_mem(size_t bytes, size_t align_bytes)
{
    static_kernel_mem_top += (size_t)static_kernel_mem_top % align_bytes;
    void *rt = static_kernel_mem_top;
    static_kernel_mem_top = (char*)static_kernel_mem_top + bytes;
    ASSERT((uint32_t)static_kernel_mem_top < STATIC_KERNEL_MEM_START
                                             + STATIC_KERNEL_MEM_SIZE);
    return rt;
}

static inline void set_bitmap32(uint32_t *bms, size_t bm_count)
{
    for(size_t i = 0;i != bm_count; ++i)
        bms[i] = 0xffffffff;
}

static inline void clr_bitmap32(uint32_t *bms, size_t bm_count)
{
    for(size_t i = 0;i != bm_count; ++i)
        bms[i] = 0x00000000;
}

static inline void set_local_bit(uint32_t *bm, size_t i)
{
    ASSERT(i < 32);
    *bm |= (1 << i);
}

static inline void clr_local_bit(uint32_t *bm, size_t i)
{
    ASSERT(i < 32);
    *bm &= ~(1 << i);
}

static inline bool get_local_bit(uint32_t bm, size_t i)
{
    ASSERT(i < 32);
    return (bm & (1 << i)) != 0;
}

static bool init_ker_phy_mem_pool(struct mem_page_pool **p_pool, size_t pool_begin, size_t pool_end)
{
    // 没有空闲物理页，这时候是不是应该assert挂掉……
    if(pool_begin >= pool_end)
    {
        *p_pool = NULL;
        return false;
    }

    // 页面总数和位图数量
    size_t pool_pages = pool_end - pool_begin;
    size_t count_L3 = ceil_int_div(pool_pages, 32);
    size_t count_L2 = ceil_int_div(count_L3, 32);
    size_t count_L1 = ceil_int_div(count_L2, 32);

    // 需要的总bitmap32数量：L1 + L2 + L3 + stay (stay与L3相同)
    size_t total_bitmap32_count = count_L1 + count_L2 + (count_L3 << 1);

    // 结构体字节数
    size_t struct_size = sizeof(struct mem_page_pool)
                       + sizeof(uint32_t) * total_bitmap32_count;
    
    struct mem_page_pool *pool = *p_pool =
                alloc_static_kernel_mem(struct_size, 4);
    
    pool->struct_size  = struct_size;
    pool->begin        = pool_begin;
    pool->end          = pool_end;
    pool->pool_pages   = pool_pages;
    pool->unused_pages = pool_pages;
    
    pool->bitmap_count_L1 = count_L1;
    pool->bitmap_count_L2 = count_L2;
    pool->bitmap_count_L3 = count_L3;

    // bitmap_data区域分割：L1 L2 L3 stay
    pool->bitmap_L1       = pool->bitmap_data;
    pool->bitmap_L2       = pool->bitmap_L1 + count_L1;
    pool->bitmap_L3       = pool->bitmap_L2 + count_L2;
    pool->bitmap_resident = pool->bitmap_L3 + count_L3;

    // 初始化物理页分配情况
    // 之前bootloader用了的都已经被排除在这个池子外了
    // 所以都初始化成未使用就行
    set_bitmap32(&pool->bitmap_L0, 1);
    set_bitmap32(pool->bitmap_L1, pool->bitmap_count_L1);
    set_bitmap32(pool->bitmap_L2, pool->bitmap_count_L2);
    set_bitmap32(pool->bitmap_L3, pool->bitmap_count_L3);
    
    // resident位图其实初不初始化无所谓……
    clr_bitmap32(pool->bitmap_resident, pool->bitmap_count_L3);

    return true;
}

void init_phy_mem_man()
{
    // 内存总量
    total_mem_bytes = *(size_t*)TOTAL_MEMORY_SIZE_ADDR;

    static_kernel_mem_top = (void*)STATIC_KERNEL_MEM_START;
    
    // 前4M物理内存已经用了，其他的都还空着
    if(!init_ker_phy_mem_pool(&phy_mem_page_pool, 0x400000 / 4096,
                              get_mem_total_bytes() / 4096))
        FATAL_ERROR("failed to initialize physical memory pool");
}

uint32_t alloc_phy_page(bool resident)
{
    // 现在是没了物理页直接挂掉
    // 以后有了调页机制和swap分区，再考虑换出到磁盘
    if(phy_mem_page_pool->unused_pages == 0)
        FATAL_ERROR("system out of memory");

    // 找到一个空闲物理页
    uint32_t idxL1          = find_lowest_nonzero_bit(phy_mem_page_pool->bitmap_L0);
    uint32_t localL2        = find_lowest_nonzero_bit(phy_mem_page_pool->bitmap_L1[idxL1]);
    uint32_t idxL2          = (idxL1 << 5) + localL2;
    uint32_t localL3        = find_lowest_nonzero_bit(phy_mem_page_pool->bitmap_L2[idxL2]);
    uint32_t idxL3          = (idxL2 << 5) + localL3;
    uint32_t local_page_idx = find_lowest_nonzero_bit(phy_mem_page_pool->bitmap_L3[idxL3]);

    // 设置L3位图中对应的标记位
    clr_local_bit(&phy_mem_page_pool->bitmap_L3[idxL3], local_page_idx);
    if(resident)
        set_local_bit(&phy_mem_page_pool->bitmap_resident[idxL3], local_page_idx);
    else
        clr_local_bit(&phy_mem_page_pool->bitmap_resident[idxL3], local_page_idx);

    // 更新L2、L1以及L0位图
    if(phy_mem_page_pool->bitmap_L3[idxL3] == 0)
    {
        clr_local_bit(&phy_mem_page_pool->bitmap_L2[idxL2], localL3);
        if(phy_mem_page_pool->bitmap_L2[idxL2] == 0)
        {
            clr_local_bit(&phy_mem_page_pool->bitmap_L1[idxL1], localL2);
            if(phy_mem_page_pool->bitmap_L1[idxL1] == 0)
                clr_local_bit(&phy_mem_page_pool->bitmap_L0, idxL1);
        }
    }

    --phy_mem_page_pool->unused_pages;
    
    return (phy_mem_page_pool->begin + (idxL3 << 5) + local_page_idx) << 12;
}

void free_phy_page(uint32_t page_phy_addr)
{
    size_t page_idx = (page_phy_addr >> 12) - phy_mem_page_pool->begin;
    ASSERT(page_idx < phy_mem_page_pool->end);

    uint32_t idxL3          = page_idx >> 5;
    uint32_t local_page_idx = page_idx & 0x1f;
    uint32_t idxL2          = idxL3 >> 5;
    uint32_t idxL1          = idxL2 >> 5;

    // 尝试释放一个没被占用的物理页，fatal error之
    if(get_local_bit(phy_mem_page_pool->bitmap_L3[idxL3], local_page_idx))
        FATAL_ERROR("freeing unused phy mem page");

    // 设置L3位图中的标记位
    set_local_bit(&phy_mem_page_pool->bitmap_L3[idxL3], local_page_idx);
    set_local_bit(&phy_mem_page_pool->bitmap_L2[idxL2], idxL3 & 0x1f);
    set_local_bit(&phy_mem_page_pool->bitmap_L1[idxL1], idxL2 & 0x1f);
    set_local_bit(&phy_mem_page_pool->bitmap_L0, idxL1);

    //resident位图无需设置，因未被使用的物理页的resident位不具有任何含义

    ++phy_mem_page_pool->unused_pages;
}

uint32_t get_free_phy_page_count()
{
    return phy_mem_page_pool->unused_pages;
}
