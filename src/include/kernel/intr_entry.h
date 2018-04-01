#ifndef TINY_OS_INTR_DESC_H
#define TINY_OS_INTR_DESC_H

#include <lib/integer.h>

// 0x20~0x2f号中断为8259A所用

#define IDT_DESC_COUNT 0x21

#define INTR_NUMBER_DIVIDE_ERROR         0
#define INTR_NUMBER_DEBUG                1
#define INTR_NUMBER_NMI                  2
#define INTR_NUMBER_BREAKPOINT           3
#define INTR_NUMBER_INT_OVERFLOW         4
#define INTR_NUMBER_BOUND_OUT            5
#define INTR_NUMBER_UNDEFINED_INSTR      6
#define INTR_NUMBER_DEVICE_NOT_AVAILABLE 7
#define INTR_NUMBER_DOUBLE_FAULT         8
#define INTR_NUMBER_NO_FLOAT_PROC        9
#define INTR_NUMBER_INVALID_TSS          10
#define INTR_NUMBER_SEG_NOT_P            11
#define INTR_NUMBER_STK_SEG_FAULT        12
#define INTR_NUMBER_GENERAL_PROTECTION   13
#define INTR_NUMBER_PAGE_FAULT           14
// 15 reserved
#define INTR_NUMBER_FLOAT_ERROR          16
#define INTR_NUMBER_ALIGNMENT_CHECK      17
#define INTR_NUMBER_MACHINE_CHECK        18
#define INTR_NUMBER_SIMD_FLOAT_EXCEPTION 19

#define INTR_NUMBER_CLOCK                32

/*
    合法的intr_function签名包括
    void function(void);
    void function(uint_t intr_number);
*/
extern void *intr_function[IDT_DESC_COUNT];

void default_intr_function(uint8_t intr_number);

void init_IDT(void);

#endif //TINY_OS_INTR_DESC_H
