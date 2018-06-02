#ifndef TINY_OS_SHARD_SCREEN_H
#define TINY_OS_SHARD_SCREEN_H

#define CH_BLACK   0b0000
#define CH_BLUE    0b0001
#define CH_GREEN   0b0010
#define CH_CYAN    0b0011
#define CH_RED     0b0100
#define CH_MAGENTA 0b0101
#define CH_BROWN   0b0110
#define CH_GRAY    0b0111

#define CH_LIGHT   0b1000

#define BG_BLACK   0b00000000
#define BG_BLUE    0b00010000
#define BG_GREEN   0b00100000
#define BG_CYAN    0b00110000
#define BG_RED     0b01000000
#define BG_MAGENTA 0b01010000
#define BG_BROWN   0b01100000
#define BG_GRAY    0b01110000

#define CH_GLIM    0b10000000

#define CON_BUF_BYTE_SIZE (80 * 25 * 2)

#define CON_BUF_CHAR_COUNT (80 * 25)

#define CON_BUF_ROW_SIZE 80
#define CON_BUF_COL_SIZE 25

/* console相关的系统调用第一个参数为操作类型，后面的才是真正的调用参数 */

/* high16：pos，low8：char */
#define CONSOLE_SYSCALL_FUNCTION_SET_CHAR        0
/* high16：pos，low8：attrib */
#define CONSOLE_SYSCALL_FUNCTION_SET_ATTRIB      1
/* high16：pos，high8(low16)：char，low8：attrib */
#define CONSOLE_SYSCALL_FUNCTION_SET_CHAR_ATTRIB 2
/* void */
#define CONSOLE_SYSCALL_FUNCTION_CLEAR_SCREEN    3

/* 80 * row + col */
#define CONSOLE_SYSCALL_FUNCTION_SET_CURSOR      4
/*
    void
    return：80 * row + col
*/
#define CONSOLE_SYSCALL_FUNCTION_GET_CURSOR      5

/* low8：char */
#define CONSOLE_SYSCALL_FUNCTION_PUT_CHAR        6
/* pointer to str */
#define CONSOLE_SYSCALL_FUNCTION_PUT_STR         7
/* void */
#define CONSOLE_SYSCALL_FUNCTION_ROLL_SCREEN     8

/* low16: pos */
#define CONSOLE_SYSCALL_FUNCTION_GET_CHAR        9

/*
    在一定y范围内滚屏
    high8(low16): beg
    low8(low16) : end
*/
#define CONSOLE_SYSCALL_FUNCTION_ROLL_SCREEN_BETWEEN 10

/* 手动指定缓存内容 */
#define CONSOLE_SYSCALL_FUNCTION_SET_BUFFER_DATA 11

/* 控制台系统系统调用功能号数量 */
#define CONSOLE_SYSCALL_FUNCTION_COUNT 12

#endif /* TINY_OS_SHARD_SCREEN_H */
