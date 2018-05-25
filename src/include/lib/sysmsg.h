#ifndef TINY_OS_LIB_SYSMSG_H
#define TINY_OS_LIB_SYSMSG_H

#include <shared/bool.h>
#include <shared/intdef.h>
#include <shared/syscall/sysmsg.h>
#include <shared/sysmsg/common.h>

/* 查询消息队列是否不为空 */
bool has_sysmsg();

/* 尝试取走一条消息，opr见shared/sysmsg/common，无消息时返回false */
bool peek_sysmsg(uint32_t opr, struct sysmsg *msg);

/* 阻塞自己，直到一条消息到来 */
void wait_for_sysmsg();

/* 注册按键消息 */
void register_key_msg();

/* 注册字符消息 */
void register_char_msg();

#endif /* TINY_OS_LIB_SYSMSG_H */
