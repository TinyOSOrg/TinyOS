#ifndef TINY_OS_FILESYS_FILESYS_H
#define TINY_OS_FILESYS_FILESYS_H

#include <shared/intdef.h>

enum filesys_opr_result
{
    filesys_opr_success,        // 操作成功完成
    filesys_opr_locked,         // 文件有互斥锁（读/写均有可能）
    filesys_opr_out_of_range,   // 超出文件大小范围
    filesys_opr_no_disk_space,  // 磁盘空间不足
    filesys_opr_read_only,      // 对只读文件进行改动
    filesys_opr_not_found,      // 文件不存在
    filesys_opr_existed,        // 已有同名文件
    filesys_opr_rm_nonempty,    // 试图删除非空目录
    filesys_opr_others,         // 其他错误
};

typedef uint32_t file_handle;

typedef uint32_t filesys_dp_handle;

/* （只读模式）打开一个文件 */
file_handle open_reading(filesys_dp_handle dp, const char *path,
                         enum filesys_opr_result *rt);

/* 以写模式打开一个文件 */
file_handle open_writing(filesys_dp_handle dp, const char *path,
                         enum filesys_opr_result *rt);

/* 关闭一个文件 */
enum filesys_opr_result close_file(filesys_dp_handle dp,
                                   file_handle file);

/* 创建一个空文件 */
enum filesys_opr_result make_regular(filesys_dp_handle dp,
                                     const char *path);

/* 删除一个文件 */
enum filesys_opr_result remove_regular(filesys_dp_handle dp,
                                       const char *path);

/* 创建一个空目录 */
enum filesys_opr_result make_directory(filesys_dp_handle dp,
                                       const char *path);

/* 删除一个空目录 */
enum filesys_opr_result remove_directory(filesys_dp_handle dp,
                                         const char *path);

#endif /* TINY_OS_FILESYS_FILESYS_H */
