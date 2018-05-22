#ifndef TINY_OS_SHARED_FILESYS_IMPORT_IMPORT_H
#define TINY_OS_SHARED_FILESYS_IMPORT_IMPORT_H

/*
    Import分区分区结构：
        uint32_t file_count: 分区中有多少个文件
        ipt_disk_file {
            uint32_t dp                  : 分区号
            path[IPT_PATH_BUF_SIZE]      : 文件路径
            uint32_t file_byte_size      : 文件字节数
            byte file[file_byte_size]    : 文件内容
        } * file_count
    注意这个结构是packed的，不要引入任何对齐padding
*/

#define IPT_PATH_BUF_SIZE 64

#endif /* TINY_OS_SHARED_FILESYS_IMPORT_IMPORT_H */
