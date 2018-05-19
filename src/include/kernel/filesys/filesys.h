#ifndef TINY_OS_FILESYS_FILESYS_H
#define TINY_OS_FILESYS_FILESYS_H

#include <shared/intdef.h>
#include <shared/filesys/filesys.h>

void init_filesys(void);

/* （只读模式）打开一个文件 */
file_handle open_regular_reading(filesys_dp_handle dp, const char *path,
                                 enum filesys_opr_result *rt);

/* 以写模式打开一个文件 */
file_handle open_regular_writing(filesys_dp_handle dp, const char *path,
                                 enum filesys_opr_result *rt);

/* 关闭一个文件 */
enum filesys_opr_result close_file(filesys_dp_handle dp, file_handle file);

/* 创建一个空文件 */
enum filesys_opr_result make_regular(filesys_dp_handle dp, const char *path);

/* 删除一个文件 */
enum filesys_opr_result remove_regular(filesys_dp_handle dp, const char *path);

/* 创建一个空目录 */
enum filesys_opr_result make_directory(filesys_dp_handle dp, const char *path);

/* 删除一个空目录 */
enum filesys_opr_result remove_directory(filesys_dp_handle dp, const char *path);

/* 取得一个文件的字节数 */
uint32_t get_regular_size(filesys_dp_handle dp, file_handle file);

/*
    向一个常规文件写入二进制内容
    设文件大小为file_size，则写入合法当且仅当
        fpos <= file_size
*/
enum filesys_opr_result write_to_regular(
                            filesys_dp_handle dp, file_handle file,
                            uint32_t fpos, uint32_t size,
                            const void *data);

/* 从常规文件中读二进制内容 */
enum filesys_opr_result read_from_regular(
                            filesys_dp_handle dp, file_handle file,
                            uint32_t fpos, uint32_t size,
                            void *data);

#endif /* TINY_OS_FILESYS_FILESYS_H */
