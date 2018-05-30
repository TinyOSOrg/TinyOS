#ifndef TINY_OS_INTERRUPT_H
#define TINY_OS_INTERRUPT_H

#include <shared/stdbool.h>
#include <shared/stdint.h>

/* 0x20~0x2f号中断为8259A所用 */

#define IDT_DESC_COUNT 0x81

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
/* 15 reserved */
#define INTR_NUMBER_FLOAT_ERROR          16
#define INTR_NUMBER_ALIGNMENT_CHECK      17
#define INTR_NUMBER_MACHINE_CHECK        18
#define INTR_NUMBER_SIMD_FLOAT_EXCEPTION 19

#define INTR_NUMBER_CLOCK                32
#define INTR_NUMBER_KEYBOARD             33

#define INTR_NUMBER_DISK                 46

/* 和linux一样，0x80号中断用作系统调用 */
#define INTR_NUMBER_SYSCALL              128

void init_IDT();

/*
    合法的intr_function签名包括
    void function();
    void function(uint32_t intr_number);
    void function(uint32_t intr_number, uint32_t err_code);
*/
void set_intr_function(uint8_t intr_number, void (*func)());

/*
    设置时钟中断频率
*/
void set_8253_freq(uint16_t freq);

/* 关于中断的状态标志 */
typedef uint32_t intr_state;

/* 标志中的中断是否打开 */
bool is_intr_on(intr_state state);

/* 取得当前计算机所处的中断状态 */
intr_state get_intr_state();

/* 设置中断状态 */
void set_intr_state(intr_state state);

/* 关中断并取得中断状态 */
intr_state fetch_and_disable_intr();

/* 开中断并取得开之前的中断状态 */
intr_state fetch_and_enable_intr();

#endif /* TINY_OS_INTERRUPT_H */
