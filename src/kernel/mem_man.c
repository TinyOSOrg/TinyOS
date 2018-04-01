#include <kernel/boot.h>
#include <kernel/mem_man.h>

size_t total_mem_bytes;

void *static_kernel_mem_top;

static struct phy_mem_pool *usr_phy_mem_pool;

void mem_man_init(void)
{
    // 内存总量
    total_mem_bytes = *(size_t*)TOTAL_MEMORY_SIZE_ADDR;

    static_kernel_mem_top = (void*)0xc0100000;
    
    //TODO：初始化usr_phy_mem_pool
}

void *alloc_static_kernel_mem(size_t bytes)
{
    void *rt = static_kernel_mem_top;
    static_kernel_mem_top += bytes;
    return rt;
}
