#ifndef TINY_OS_EXPLORER_EXPLORER_H
#define TINY_OS_EXPLORER_EXPLORER_H

/* Explorer入口，用于创建explorer线程 */
void explorer();

/* 当一个前台进程被kill时，需要通过该函数来通知explorer */
void foreground_exit();

#endif /* TINY_OS_EXPLORER_EXPLORER_H */
