#ifndef TINY_OS_LIB_FREELIST_H
#define TINY_OS_LIB_FREELIST_H

#include <lib/stdbool.h>
#include <lib/stdint.h>

/* 自由链表句柄 */
typedef uint32_t freelist_handle;

/* 初始化一个自由链表 */
void init_freelist(freelist_handle *handle);

/*
    向自由链表中插入一块区域
    区域大小应不小于sizeof(void*)
*/
void add_freelist(freelist_handle *handle, void *mem_zone);

/* 一个自由链表是否为空 */
bool is_freelist_empty(freelist_handle *handle);

/*
    从自由链表中取出一块区域
    链表为空时返回NULL
*/
void *fetch_freelist(freelist_handle *handle);

#endif /* TINY_OS_LIB_FREELIST_H */
