#ifndef TINY_OS_SHARD_PATH_H
#define TINY_OS_SHARD_PATH_H

#include <shared/filesys.h>
#include <shared/stdbool.h>
#include <shared/stdint.h>

/* 常见的关于路径字符串的操作 */

/* 判断一个路径是否是绝对路径 */
bool is_absolute_path(const char *path);

/* 判断一个路径是否指定了分区 */
bool is_path_containning_dp(const char *path);

/* 从一个路径中提取出分区名，返回实际复制的字符数 */
uint32_t get_dp_from_path_s(const char *path,
                            char *dst, uint32_t buf_size);

/* 跳过一个路径中的分区部分，要求输入是包含分区的 */
const char *skip_dp_in_abs_path(const char *path);

/*
    给定当前路径和delta路径，取得叠加后的结果
    dst不能含有分区
    缓冲区不够时返回false
*/
bool cat_path_s(const char *src, const char *delta,
                char *dst, uint32_t buf_size);

/* 同上，但会考虑分区的切换 */
bool cat_path_ex_s(filesys_dp_handle src_dp, const char *src_path,
                   const char *delta,
                   filesys_dp_handle *out_dp, char *out_path,
                   uint32_t out_path_buf_size);

/* 给定一个文件路径，将其转换为父目录路径 */
bool to_parent_dir(char *filepath);

#endif /* TINY_OS_SHARD_PATH_H */
