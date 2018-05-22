#ifndef TINY_OS_TOOLS_DISK_H
#define TINY_OS_TOOLS_DISK_H

/*
    外部工具中有很多是关于磁盘映像操作的
    这里定义它们的共有参数
*/

/* 磁盘最大扇区号 */
#define DISK_MAX_SECTOR_NUMBER 262143

/* import分区起始扇区 */
#define DISK_IMPORT_DP_BEGIN 182144

/* import分区结束扇区 */
#define DISK_IMPORT_DP_END DISK_MAX_SECTOR_NUMBER

#endif /* TINY_OS_TOOLS_DISK_H */
