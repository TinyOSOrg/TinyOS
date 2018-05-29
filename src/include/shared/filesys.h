#ifndef TINY_OS_SHARED_FILESYS_FILESYS_H
#define TINY_OS_SHARED_FILESYS_FILESYS_H

#include <shared/atrc.h>

#include <lib/stdint.h>

/*================================================================================
    文件系统相关
================================================================================*/

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

/*================================================================================
    磁盘分区类型
    取值见下方宏定义
================================================================================*/
typedef uint32_t disk_partition_type;

#define DISK_PT_NONEXISTENT 0   /* 不存在（分区表项无效） */
#define DISK_PT_NOFS        1   /* 存在但未格式化 */
#define DISK_PT_SWAP        2   /* 交换分区 */
#define DISK_PT_IMPORT      3   /* 文件交换分区，用于测试时从外部将文件导入虚拟机 */
#define DISK_PT_AFS         4   /* 建立了AFS */

#define DISK_PT_FORMATTING  255 /* 格式化中，禁止访问 */

/* 分区名字最大长度 */
#define DP_NAME_BUF_SIZE 16

/* 分区表项数，注意整个分区表不要超过一个扇区大小 */
#define DPT_UNIT_COUNT 16

/* 分区表单位结构 */
struct dpt_unit
{
    disk_partition_type type;
    uint32_t sector_begin;
    uint32_t sector_end;
    char name[DP_NAME_BUF_SIZE];
};

/* 分区表本身位于ata0磁盘中的LBA扇区号 */
#define DPT_SECTOR_POSITION 300

/*================================================================================
    Import分区分区结构：
        uint32_t file_count: 分区中有多少个文件
        ipt_disk_file {
            uint32_t dp                  : 分区号
            path[IPT_PATH_BUF_SIZE]      : 文件路径
            uint32_t file_byte_size      : 文件字节数
            byte file[file_byte_size]    : 文件内容
        } * file_count
    注意这个结构是packed的，不要引入任何对齐padding
================================================================================*/

#define IPT_PATH_BUF_SIZE 64

/*================================================================================
    文件系统相关的系统调用的参数结构体
================================================================================*/

struct syscall_filesys_open_params
{
    bool writing;
    filesys_dp_handle dp;
    const char *path;
    usr_file_handle *result;
};

struct syscall_filesys_read_params
{
    usr_file_handle file;
    uint32_t fpos, byte_size;
    void *data_dst;
};

struct syscall_filesys_write_params
{
    usr_file_handle file;
    uint32_t fpos, byte_size;
    const void *data_src;
};

struct syscall_filesys_get_child_info_params
{
    filesys_dp_handle dp;
    const char *path;
    uint32_t idx;
    struct syscall_filesys_file_info *info;
};

struct syscall_filesys_file_info
{
    bool is_dir;
    char name[FILE_NAME_MAX_LEN + 1];
};

#endif /* TINY_OS_SHARED_FILESYS_FILESYS_H */
