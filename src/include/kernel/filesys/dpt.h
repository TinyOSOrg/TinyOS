#ifndef TINY_OS_DISK_PARTITION_TABLE_H
#define TINY_OS_DISK_PARTITION_TABLE_H

#include <shared/filesys/dpt.h>

void init_dpt(void);

const struct dpt_unit *get_dpt_unit(size_t idx);

#endif /* TINY_OS_DISK_PARTITION_TABLE_H */
