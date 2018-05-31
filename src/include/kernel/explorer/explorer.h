#ifndef TINY_OS_EXPLORER_EXPLORER_H
#define TINY_OS_EXPLORER_EXPLORER_H

#include <shared/stdint.h>

struct PCB;

/* Explorer入口，用于创建explorer线程 */
void explorer();

/* 当一个前台进程被kill时，需要通过该函数来通知explorer */
void foreground_exit(struct PCB *pcb);

/* 系统调用实现：申请成为前台进程 */
uint32_t syscall_alloc_fg_impl();

/* 系统调用实现：若自己处于前台，把自己切下去 */
uint32_t syscall_free_fg_impl();

/* 系统调用实现：申请屏幕显示缓存 */
uint32_t syscall_alloc_con_buf_impl();

/* 系统调用实现：在disp区域输出一个字符 */
uint32_t syscall_put_char_expl_impl(uint32_t arg);

/* 系统调用实现：让disp区域开个新行 */
uint32_t syscall_expl_new_line_impl();

#endif /* TINY_OS_EXPLORER_EXPLORER_H */
