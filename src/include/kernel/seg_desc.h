#ifndef TINY_OS_SEG_DESC_H
#define TINY_OS_SEG_DESC_H

/* 段选择子的基本构成 */

#define SEG_SEL_RPL_0 0x0
#define SEG_SEL_RPL_1 0x1
#define SEG_SEL_RPL_2 0x2
#define SEG_SEL_RPL_3 0x3

#define SEG_SEL_USE_GDT (0 << 2)
#define SEG_SEL_USE_LDT (1 << 2)

#define SEG_SEL_KERNEL_CODE ((1 << 3) | SEG_SEL_USE_GDT | SEG_SEL_RPL_0)
#define SEG_SEL_KERNEL_DATA ((2 << 3) | SEG_SEL_USE_GDT | SEG_SEL_RPL_0)
#define SEG_SEL_KERNEL_VIDEO ((3 << 3) | SEG_SEL_USE_GET | SEG_SEL_RPL_0)
#define SEG_SEL_KERNEL_STACK SEG_SEL_KERNEL_DATA

#endif /* TINY_OS_SEG_DESC_H */
