#ifndef TINY_OS_DISK_PARTITION_TABLE_H
#define TINY_OS_DISK_PARTITION_TABLE_H

#include <shared/filesys/dpt.h>

/* 初始化分区表 */
void init_dpt(void);

/* 取得第idx个分区表项 */
struct dpt_unit *get_dpt_unit(size_t idx);

/* 将分区表写回到磁盘 */
void restore_dpt(void);

#endif /* TINY_OS_DISK_PARTITION_TABLE_H */
