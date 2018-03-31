#ifndef TINY_OS_INTR_DESC_H
#define TINY_OS_INTR_DESC_H

#include <lib/integer.h>

#define IDT_DESC_COUNT 0x21

extern void (*intr_entry_function[IDT_DESC_COUNT])(uint8_t intr_number);

void init_IDT(void);

#endif //TINY_OS_INTR_DESC_H
