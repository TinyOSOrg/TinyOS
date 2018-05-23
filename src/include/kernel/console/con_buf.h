#ifndef TINY_OS_CONSOLE_CON_BUF_H
#define TINY_OS_CONSOLE_CON_BUF_H

#include <kernel/process/semaphore.h>

#include <shared/intdef.h>

#define CON_BUF_BYTE_SIZE (80 * 25 * 2)

#define CON_BUF_CHAR_COUNT (80 * 25)

#define CON_BUF_ROW_SIZE 80
#define CON_BUF_COL_SIZE 25

struct con_buf
{
    char data[CON_BUF_BYTE_SIZE];
    struct semaphore lock;
    uint16_t cursor;
};

struct con_buf *alloc_con_buf();

void free_con_buf(struct con_buf *buf);

#endif /* TINY_OS_CONSOLE_CON_BUF_H */
