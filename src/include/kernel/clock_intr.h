#ifndef TINY_OS_CLOCK_INTR_H
#define TINY_OS_CLOCK_INTR_H

#include <lib/intdef.h>

/*
    设置时钟中断频率
*/
void set_8253_freq(uint16_t freq);

#endif /* TINY_OS_CLOCK_INTR_H */
