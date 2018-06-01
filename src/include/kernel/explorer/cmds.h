#ifndef TINY_OS_EXPLORER_PROCS_H
#define TINY_OS_EXPLORER_PROCS_H

#include <kernel/filesys/filesys.h>

#include <shared/stdint.h>

/* Explorer中的部分内建命令 */

/* 输出进程列表 */
void expl_show_procs();

/* cd，失败时返回false */
bool expl_cd(filesys_dp_handle *dp, char *cur,
             uint32_t *len, uint32_t max_len,
             const char *arg);

/*
    exec，执行一个用户程序
    args为
        程序路径（绝对/相对） 参数1 参数2 ……
*/
bool expl_exec(filesys_dp_handle dp, const char *working_dir,
               const char *dst, const char *proc_name,
               const char **args, uint32_t args_cnt,
               uint32_t *_pid);

/* dp，显示当前分区名 */
void expl_dp(filesys_dp_handle dp);

/* mkdir，不解释 */
void expl_mkdir(filesys_dp_handle dp, const char *working_dir,
                const char *dirname);

/* rmdir，不解释 */
void expl_rmdir(filesys_dp_handle dp, const char *working_dir,
                const char *dirname);

/* rmfile，不解释 */
void expl_rmfile(filesys_dp_handle dp, const char *working_dir,
                 const char *filename);

#endif /* TINY_OS_EXPLORER_PROCS_H */
