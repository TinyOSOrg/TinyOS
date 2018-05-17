#ifndef TINY_OS_FILESYS_FILESYS_H
#define TINY_OS_FILESYS_FILESYS_H

#include <shared/intdef.h>

/* 关闭一个文件 */
void close_file(size_t dp_idx, uint32_t file_handle);

#endif /* TINY_OS_FILESYS_FILESYS_H */
