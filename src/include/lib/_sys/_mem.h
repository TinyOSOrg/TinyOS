#ifndef TINY_OS_LIB_SYS_dMEM_H
#define TINY_OS_LIB_SYS_dMEM_H

#include <shared/proc_mem.h>
#include <shared/stdint.h>

void _init_mem_man();

/* 返回的结果为4字节对齐，失败时返回NULL */
void *_malloc(size_t size);

/* ptr为调用_malloc得到的结果 */
void _free(void *ptr);

#endif /* TINY_OS_LIB_SYS_dMEM_H */
