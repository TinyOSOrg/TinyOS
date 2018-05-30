#ifndef TINY_OS_CONSOLE_CON_BUF_H
#define TINY_OS_CONSOLE_CON_BUF_H

#include <kernel/process/semaphore.h>

#include <shared/screen.h>
#include <shared/stdint.h>

struct con_buf
{
    char data[CON_BUF_BYTE_SIZE];
    struct semaphore lock;
    uint16_t cursor;
};

struct con_buf *alloc_con_buf();

void free_con_buf(struct con_buf *buf);

#endif /* TINY_OS_CONSOLE_CON_BUF_H */
