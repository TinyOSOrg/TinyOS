#ifndef TINY_OS_SHARED_FILESYS_DPT_H
#define TINY_OS_SHARED_FILESYS_DPT_H

#include <shared/intdef.h>

/*
    磁盘分区类型
    取值见下方宏定义
*/
typedef uint32_t disk_partition_type;

#define DISK_PT_NONEXISTENT 0   /* 不存在（分区表项无效） */
#define DISK_PT_NOFS        1   /* 存在但未格式化 */
#define DISK_PT_SWAP        2   /* 交换分区 */
#define DISK_PT_AFS         3   /* 建立了AFS */

/* 分区名字最大长度 */
#define DP_NAME_BUF_SIZE 16

/* 分区表项数，注意整个分区表不要超过一个扇区大小 */
#define DPT_UNIT_COUNT 16

/* 分区表单位结构 */
struct dpt_unit
{
    disk_partition_type type;
    uint32_t sector_begin;
    uint32_t sector_end;
    char name[DP_NAME_BUF_SIZE];
};

/* 分区表本身位于ata0磁盘中的LBA扇区号 */
#define DPT_SECTOR_POSITION 300

#endif /* TINY_OS_SHARED_FILESYS_DPT_H */
