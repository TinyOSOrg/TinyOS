#ifndef TINY_OS_SEG_DESC_H
#define TINY_OS_SEG_DESC_H

#include <lib/stdint.h>

/*
    段描述符的基本构成
    和src/boot/boot.s中的定义基本一致，含义参见之

    这个文件中的东西基本没有原创的（
*/

#define SEG_DESC_G_4K (1 << 7)
#define SEG_DESC_D_32 (1 << 6)
#define SEG_DESC_L_32 (0 << 5)
#define SEG_DESC_AVL  (0 << 4)

#define SEG_DESC_P     (1 << 7)
#define SEG_DESC_DPL_0 (0 << 5)
#define SEG_DESC_DPL_3 (3 << 5)

#define SEG_DESC_S_SYS    (0 << 4)
#define SEG_DESC_S_NONSYS (1 << 4)

#define SEG_DESC_TYPE_CODE 8
#define SEG_DESC_TYPE_DATA 2
#define SEG_DESC_TYPE_TSS  9

/* 任务状态段（Task State Segment）描述符基本构成 */

#define TSS_DESC_D    (0 << 6)

/* 段选择子的基本构成 */

#define SEG_SEL_RPL_0 0x0
#define SEG_SEL_RPL_1 0x1
#define SEG_SEL_RPL_2 0x2
#define SEG_SEL_RPL_3 0x3

#define SEG_SEL_USE_GDT (0 << 2)
#define SEG_SEL_USE_LDT (1 << 2)

/* 内核代码段，数据段，栈段 */

#define SEG_SEL_KERNEL_CODE ((1 << 3) | SEG_SEL_USE_GDT | SEG_SEL_RPL_0)
#define SEG_SEL_KERNEL_DATA ((2 << 3) | SEG_SEL_USE_GDT | SEG_SEL_RPL_0)
#define SEG_SEL_KERNEL_STACK SEG_SEL_KERNEL_DATA

/* TSS选择子 */

#define TSS_SEL ((3 << 3) | SEG_SEL_USE_GDT | SEG_SEL_RPL_0)

/* 用户代码段、数据段 */

#define SEG_SEL_USER_CODE  ((4 << 3) | SEG_SEL_USE_GDT | SEG_SEL_RPL_3)
#define SEG_SEL_USER_DATA  ((5 << 3) | SEG_SEL_USE_GDT | SEG_SEL_RPL_3)
#define SEG_SEL_USER_STACK SEG_SEL_USER_DATA

#define GDT_ATTRIB_HIGH (SEG_DESC_G_4K | \
                         SEG_DESC_D_32 | \
                         SEG_DESC_L_32 | \
                         SEG_DESC_AVL)
#define GDT_ATTRIB_LOW_CODE (SEG_DESC_P | \
                             SEG_DESC_DPL_3 | \
                             SEG_DESC_S_NONSYS | \
                             SEG_DESC_TYPE_CODE)
#define GDT_ATTRIB_LOW_DATA (SEG_DESC_P | \
                             SEG_DESC_DPL_3 | \
                             SEG_DESC_S_NONSYS | \
                             SEG_DESC_TYPE_DATA)
#define TSS_ATTRIB_HIGH (SEG_DESC_G_4K | \
                         TSS_DESC_D | \
                         SEG_DESC_L_32 | \
                         SEG_DESC_AVL)
#define TSS_ATTRIB_LOW (SEG_DESC_P | \
                        SEG_DESC_DPL_0 | \
                        SEG_DESC_S_SYS | \
                        SEG_DESC_TYPE_TSS)

/*
    GDT表项
    小端序真方便，赞美x86！
*/
struct iGDT
{
    uint32_t low;
    uint32_t high;
};

static inline void make_iGDT(struct iGDT *output,
                             uint32_t base, uint32_t limit,
                             uint8_t attrib_low, uint8_t attrib_high)
{
    output->low  = ((base & 0x0000ffff) << 16) | (limit & 0x0000ffff);
    output->high = ((base >> 24) << 24) |
        ((((limit & 0x000f0000) >> 16) + attrib_high) << 16) |
        (attrib_low << 8) | ((base & 0x00ff0000) >> 16);
}

#endif /* TINY_OS_SEG_DESC_H */
