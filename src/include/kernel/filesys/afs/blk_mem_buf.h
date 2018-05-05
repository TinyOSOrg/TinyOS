#ifndef TINY_OS_FILESYS_AFS_BLK_MEM_BUF_H
#define TINY_OS_FILESYS_AFS_BLK_MEM_BUF_H

/* 用来分配block内存缓冲 */

void init_afs_block_buffer_allocator(void);

void *afs_alloc_block_buffer(void);

void afs_free_block_buffer(void *buf);

#endif /* TINY_OS_FILESYS_AFS_BLK_MEM_BUF_H */
