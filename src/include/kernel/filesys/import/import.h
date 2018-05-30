#ifndef TINY_OS_FILESYS_IMPORT_IMPORT_H
#define TINY_OS_FILESYS_IMPORT_IMPORT_H

#include <shared/stdint.h>

/*
    Import分区是用于开发和测试的一种特殊磁盘分区

    由于TinyOS还没有任何获取虚拟机外的文件的手段，要获取从外部交叉编译得到的程序，直接以flat模式从磁盘映像上获取是最为方便的
    Import分区就是用来做这个的分区……

    Import分区的结构定义在shared/filesys/import/import.h中
*/

/* 从以beg开头的import分区中导入外部文件 */
void ipt_import_from_dp(uint32_t dp_beg);

#endif /* TINY_OS_FILESYS_IMPORT_IMPORT_H */
