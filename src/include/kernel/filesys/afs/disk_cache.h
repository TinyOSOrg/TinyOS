#ifndef TINY_OS_FILESYS_AFS_SECTOR_CACHE_H
#define TINY_OS_FILESYS_AFS_SECTOR_CACHE_H

#include <shared/intdef.h>

/*
    进程对磁盘的访问通常也是有时空局部性的，因此这里做个LRU的磁盘访问cache，不考虑抖动什么的
*/

/*
    提供一个全局磁盘读写缓冲区，给定扇区号/块号和块内偏移、长度，能够访问磁盘上该块的数据

    缓冲区基于linked map实现，采用LRU策略管理

    块的读写缓冲用于文件读写
    扇区的读写缓冲应该主要是用于entry访问
    要求扇区请求和块请求不能有任何重叠，否则UB

    扇区有读写和读读互斥：
        如果扇区A被读入导致扇区B需要被释放，那么由读入A的线程将B标记为待释放。此时：

            如果B没有操作者，就直接由读入A的线程把B释放、写回
            如果B有操作者，就由最后一个离开的操作者将B释放

            读者计数器和销毁标记均用关中断保护
        
        读者写者都算是操作者。

    块则没有互斥，因为块读写用于文件操作，应该由文件系统负责互斥
*/

void init_afs_disk_cache(void);

void afs_read_from_sector(uint32_t sec, size_t offset, size_t size, void *data);

void afs_write_to_sector(uint32_t sec, size_t offset, size_t size, const void *data);

/*
    释放所有扇区缓存
    比如关机的时候/销毁文件系统的时候用
    调用时，文件系统须保证没有别的线程会调用afs_read/write_from/to_sector函数
*/
void afs_release_all_sector_cache(void);

/* 获取某个扇区的缓存区以用于读取，用完一定要调用对应的end函数 */
const void *afs_read_from_sector_begin(uint32_t sec);

void afs_read_from_sector_end(uint32_t sec);

/* 获取某个扇区的缓存区以用于写入，用完一定要调用对应的end函数 */
void *afs_write_to_sector_begin(uint32_t sec);

void afs_write_to_sector_end(uint32_t sec);

void afs_read_from_block(uint32_t sec, size_t offset, size_t size, void *data);

void afs_write_to_block(uint32_t sec, size_t offset, size_t size, const void *data);

void afs_release_all_block_cache(void);

const void *afs_read_from_block_begin(uint32_t sec);

void afs_read_from_block_end(uint32_t sec);

void *afs_write_to_block_begin(uint32_t sec);

void afs_write_to_block_end(uint32_t sec);

#endif /* TINY_OS_FILESYS_AFS_SECTOR_CACHE_H */
