#ifndef TINY_OS_DISK_PARTITION_TABLE_H
#define TINY_OS_DISK_PARTITION_TABLE_H

#include <shared/filesys/dpt.h>
#include <shared/filesys/filesys.h>

#include <lib/stdbool.h>

/* 初始化分区表 */
void init_dpt();

/* 取得第idx个分区表项 */
struct dpt_unit *get_dpt_unit(size_t idx);

/* 将一个分区格式化为指定的类型 */
bool reformat_dp(size_t idx, disk_partition_type type);

/* 取得某个分区文件系统的句柄 */
uint32_t get_dp_fs_handler(size_t idx);

/* 由名字取得分区句柄，失败时返回DPT_UNIT_COUNT */
filesys_dp_handle get_dp_handle_by_name(const char *name);

/* 将分区表写回到磁盘 */
void restore_dpt();

/* 销毁分区模块内容，顺带会销毁文件系统句柄 */
void destroy_dpt();

#endif /* TINY_OS_DISK_PARTITION_TABLE_H */
