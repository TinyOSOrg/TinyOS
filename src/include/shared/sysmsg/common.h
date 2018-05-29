#ifndef TINY_OS_SHARED_SYSMSG_COMMON_H
#define TINY_OS_SHARED_SYSMSG_COMMON_H

#include <lib/stdint.h>

/*
    内核进程消息类型
    只能是预定义好的，并不能运行时注册增删
*/
typedef uint32_t sysmsg_type;

/* 合法的消息类型 */

#define SYSMSG_TYPE_KEYBOARD 0 /* 键盘按下或释放，参数定义在kernel/kbdriver.h中 */

#define SYSMSG_TYPE_CHAR     1 /* 字符输入消息，参数定义在kernel/kbdriver.h中 */

/*
    内核消息参数字节数
    消息传递并不是拿来大规模发送数据的，所以允许的参数就很有限了……
    至于这块空间如何解释，每种消息有自己的约定
*/
#define SYSMSG_PARAM_SIZE 12

/* 内核进程消息 */
struct sysmsg
{
    sysmsg_type type;
    uint8_t params[SYSMSG_PARAM_SIZE];
};

#endif /* TINY_OS_SHARED_SYSMSG_COMMON_H */
