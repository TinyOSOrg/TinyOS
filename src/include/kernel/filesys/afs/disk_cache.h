#ifndef TINY_OS_FILESYS_AFS_DISK_CACHE_H
#define TINY_OS_FILESYS_AFS_DISK_CACHE_H

#include <shared/intdef.h>

/*
    进程对磁盘的访问通常也是有时空局部性的，因此这里做个LRU的磁盘访问cache，不考虑抖动什么的
*/

/*
    提供一个全局磁盘读写缓冲区，给定扇区号/块号和块内偏移、长度，能够访问磁盘上该块的数据
    缓冲区基于linked map实现，采用LRU策略管理

    如果块A被读入导致块B需要被释放，那么由读入A的线程将B标记为待释放。此时：

        如果B没有操作者，就直接由读入A的线程把B释放、写回
        如果B有操作者，就由最后一个离开的操作者将B释放

        读者计数器和销毁标记均用关中断保护
    
    块的读写互斥：按经典的读者优先方案处理。读者写者都算是操作者。

    块的读写缓冲应该主要是用于文件读写
    扇区的读写缓冲应该主要是用于entry访问
    要求扇区请求和块请求不能有任何重叠，否则UB
*/

void init_afs_disk_cache(void);

void afs_read_from_sector(uint32_t sec, size_t offset, size_t size, void *data);

void afs_write_to_sector(uint32_t sec, size_t offset, size_t size, const void *data);

void afs_read_from_block(uint32_t beg, size_t offset, size_t size, void *data);

void afs_write_to_block(uint32_t beg, size_t offset, size_t size, const void *data);

#endif /* TINY_OS_FILESYS_AFS_DISK_CACHE_H */
