#include <kernel/asm.h>
#include <kernel/assert.h>

#include <kernel/memory/vir_page_man.h>

#include <lib/bool.h>
#include <lib/string.h>

struct vir_page_man
{
    size_t begin, end;
    size_t total, unused;

    // IMPROVE：没错，暴力位图
    // 为1表示可用，为0表示已占用
    uint32_t bitmap[0];
};

size_t get_vir_page_man_byte_size(size_t pool_begin, size_t pool_end)
{
    ASSERT_S(pool_begin < pool_end);
    
    return ((pool_end - pool_begin) / 32) * sizeof(uint32_t)
         + sizeof(struct vir_page_man);
}

void init_vir_page_man(struct vir_page_man *page_man,
                       size_t pool_begin, size_t pool_end)
{
    ASSERT_S(page_man != NULL);
    ASSERT_S(pool_begin < pool_end);

    page_man->begin  = pool_begin;
    page_man->end    = pool_end;
    page_man->total  = pool_end - pool_begin;
    page_man->unused = page_man->total;

    memset((char*)page_man->bitmap, 0xff, page_man->total / 32 * sizeof(uint32_t));
}

uint32_t alloc_page_in_vir_addr_space(struct vir_page_man *page_man)
{
    ASSERT_S(page_man != NULL);
    if(page_man->unused == 0)
        return 0;
    
    for(size_t bmp_idx = 0; ; ++bmp_idx)
    {
        if(page_man->bitmap[bmp_idx] != 0)
        {
            uint32_t loc_idx = _find_lowest_nonzero_bit(page_man->bitmap[bmp_idx]);
            page_man->bitmap[bmp_idx] &= ~(1 << loc_idx);
            --page_man->unused;
            return (((bmp_idx << 5) + loc_idx) + page_man->begin) << 12;
        }
    }
    return 0;
}

void free_page_in_vir_addr_space(struct vir_page_man *page_man, uint32_t addr)
{
    ASSERT_S(page_man != NULL);
    ASSERT_S(page_man->unused < page_man->total);
    ++page_man->unused;
    uint32_t idx = (addr >> 12) - page_man->begin;
    page_man->bitmap[idx >> 5] |= (1 << (idx & 0x1f));
}
