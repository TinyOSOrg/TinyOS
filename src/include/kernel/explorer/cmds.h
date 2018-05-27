#ifndef TINY_OS_EXPLORER_PROCS_H
#define TINY_OS_EXPLORER_PROCS_H

#include <kernel/filesys/filesys.h>

/* Explorer中的部分内建命令 */

/* 输出进程列表 */
void expl_show_procs();

/* ls，如果打开路径失败，返回false */
bool expl_ls(filesys_dp_handle dp, const char *dir);

#endif /* TINY_OS_EXPLORER_PROCS_H */
