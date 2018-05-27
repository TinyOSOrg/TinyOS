#ifndef TINY_OS_SHARED_FILESYS_FILESYS_H
#define TINY_OS_SHARED_FILESYS_FILESYS_H

#include <shared/atrc.h>
#include <shared/intdef.h>

/*
    统一的文件系统接口
*/

enum filesys_opr_result
{
    filesys_opr_success,           // 操作成功完成
    filesys_opr_locked,            // 文件有互斥锁（读/写均有可能）
    filesys_opr_out_of_range,      // 超出文件大小范围
    filesys_opr_no_disk_space,     // 磁盘空间不足
    filesys_opr_read_only,         // 对只读文件进行改动
    filesys_opr_not_found,         // 文件不存在
    filesys_opr_existed,           // 已有同名文件
    filesys_opr_rm_nonempty,       // 试图删除非空目录
    filesys_opr_invalid_dp,        // 非法文件分区
    filesys_opr_file_table_full,   // 进程打开的文件数过多
    filesys_opr_invalid_args,      // 参数非法
    filesys_opr_others,            // 其他错误
};

#define FILE_NAME_MAX_LEN 63

typedef uint32_t file_handle;

typedef uint32_t filesys_dp_handle;

typedef atrc_elem_handle usr_file_handle;

#endif /* TINY_OS_SHARED_FILESYS_FILESYS_H */
