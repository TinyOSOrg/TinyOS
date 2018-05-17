#ifndef TINY_OS_FILESYS_AFS_FILE_OPR_H
#define TINY_OS_FILESYS_AFS_FILE_OPR_H

enum afs_file_operation_status
{
    afs_file_opr_success,           // 操作正常完成
    afs_file_opr_writing_lock,      // 试图打开一个有写入锁的文件
    afs_file_opr_reading_lock,      // 试图以读写模式打开一个有读取锁的文件
    afs_file_opr_not_opening,       // 要关闭的文件并不在已打开文件列表中
    afs_file_opr_limit_exceeded,    // 文件读写范围超出文件大小
    afs_file_opr_no_empty_entry,    // 创建文件失败：没有空闲的entry
    afs_file_opr_no_empty_block,    // 创建/扩充文件失败：没有空闲的block
    afs_file_opr_read_only,         // 试图写入一个只读文件
    afs_file_opr_invalid_new_size,  // 扩充文件时，新的大小不是合法值
    afs_file_opr_not_found,         // 文件不存在
    afs_file_opr_rm_nonempty,       // 试图删除一个非空文件夹
    afs_file_opr_file_existed,      // 试图创建一个与其他某个文件同名的文件
    afs_file_opr_rm_locked,         // 试图删除一个有人在用的文件
    afs_file_opr_rm_wrong_type,     // 删除文件时指定的类型不匹配
};

#endif /* TINY_OS_FILESYS_AFS_FILE_OPR_H */
