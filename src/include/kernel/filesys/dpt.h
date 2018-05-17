#ifndef TINY_OS_DISK_PARTITION_TABLE_H
#define TINY_OS_DISK_PARTITION_TABLE_H

#include <shared/bool.h>
#include <shared/filesys/dpt.h>

/* 初始化分区表 */
void init_dpt();

/* 取得第idx个分区表项 */
struct dpt_unit *get_dpt_unit(size_t idx);

/* 将一个分区格式化为指定的类型 */
bool reformat_dp(size_t idx, disk_partition_type type);

/* 取得某个分区文件系统的句柄 */
uint32_t get_dp_fs_handler(size_t idx);

/* 将分区表写回到磁盘 */
void restore_dpt();

/* 关闭一个文件 */
void close_file(size_t dp_idx, uint32_t file_handle);

#endif /* TINY_OS_DISK_PARTITION_TABLE_H */
