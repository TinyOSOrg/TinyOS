#ifndef TINY_OS_DISK_PARTITION_TABLE_H
#define TINY_OS_DISK_PARTITION_TABLE_H

#include <shared/filesys/dpt.h>

void init_dpt(void);

void rewrite_dpt(void);

#endif /* TINY_OS_DISK_PARTITION_TABLE_H */
