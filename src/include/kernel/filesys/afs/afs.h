#ifndef TINY_OS_FILESYS_AFS_AFS_H
#define TINY_OS_FILESYS_AFS_AFS_H

#include <kernel/filesys/dpt.h>

/* afs分区句柄 */
struct afs_handle;

/* 初始化整个afs系统 */
void init_afs(void);

/*
    在一个分区上新构建一个afs文件系统
    不管原来是什么
*/
struct afs_handle *construct_new_afs(const struct dpt_unit *unit);

/* 给定一个已经建立了afs的分区，在上面加载一个handle */
struct afs_handle *init_afs_handle(const struct dpt_unit *unit);

/* 释放一个afs handle空间 */
void release_afs_handle(struct afs_handle *handle);

#endif /* TINY_OS_FILESYS_AFS_AFS_H */
