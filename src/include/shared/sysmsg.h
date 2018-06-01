#ifndef TINY_OS_SHARD_SYSMSG_H
#define TINY_OS_SHARD_SYSMSG_H

#include <shared/stdbool.h>
#include <shared/stdint.h>

/*
    每个进程有自己的系统消息队列
    下面是于此相关的系统调用约定
*/

/*
    各种消息队列操作共用同一个系统调用号，所以第一个参数是功能号
    后面的参数含义由功能号定义
*/

/* 本进程系统消息队列是否为空，无参数 */
#define SYSMSG_SYSCALL_FUNCTION_IS_EMPTY              0

/*
    第一个参数：对消息的处理方式，可按位或，见下面的各种PEEK_OPERATION
    第二个参数：输出缓冲区在用户虚拟地址空间中的地址
    返回值：是否peek到消息
*/
#define SYSMSG_SYSCALL_FUNCTION_PEEK_MSG              1

#define SYSMSG_SYSCALL_PEEK_OPERATION_REMOVE 1

/* 注册接受键盘消息，无参数 */
#define SYSMSG_SYSCALL_FUNCTION_REGISTER_KEYBOARD_MSG 2

/* 注册接受字符消息，无参数 */
#define SYSMSG_SYSCALL_FUNCTION_REGISTER_CHAR_MSG     3

/* 将当前线程阻塞在进程消息队列中，无参数 */
#define SYSMSG_SYSCALL_FUNCTION_BLOCK_ONTO_SYSMSG     4

#define SYSMSG_SYSCALL_FUNCTION_COUNT 5

/*
    内核进程消息类型
    只能是预定义好的，并不能运行时注册增删
*/
typedef uint32_t sysmsg_type;

/* 合法的消息类型 */

#define SYSMSG_TYPE_KEYBOARD 0 /* 键盘按下或释放，参数定义在kernel/kbdriver.h中 */

#define SYSMSG_TYPE_CHAR     1 /* 字符输入消息，参数定义在kernel/kbdriver.h中 */

#define SYSMSG_TYPE_EXPL_INPUT 2 /* 来自explorer的输入 */

#define SYSMSG_TYPE_EXPL_NEW_LINE 3 /* 让explorer开个新行 */

#define SYSMSG_TYPE_EXPL_OUTPUT 4 /* 到explorer的输出 */

#define SYSMSG_TYPE_PIPE_NULL_CHAR 5 /* 设有进程 A | B，A调用该函数会让B得到一个空字符 */

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

#endif /* TINY_OS_SHARD_SYSCALL_SYSMSG_H */
