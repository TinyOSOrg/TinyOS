#ifndef TINY_OS_NFS_BCACHE_H
#define TINY_OS_NFS_BCACHE_H

#include <shared/stdint.h>

#define NFS_BLK_SEC_CNT 4
#define NFS_SEC_SIZE    512
#define NFS_BLK_SIZE    (NFS_BLK_SEC_CNT * NFS_SEC_SIZE)

#define NFS_PG_SIZE     4096

void nfs_init_bcache();

/* 申请一块block缓存 */
void *nfs_balloc();

/* 释放一块block缓存 */
void nfs_bfree(void *buf);

/* 开始读取磁盘上的block */
void nfs_rb_beg(uint32_t sec);

/* 结束读取磁盘上的block */
void nfs_rb_end(uint32_t sec);

/* 开始读写磁盘上的block */
void nfs_wb_beg(uint32_t sec);

/* 结束读写磁盘上的block */
void nfs_rb_end(uint32_t sec);

#endif /* TINY_OS_NFS_BCACHE_H */
