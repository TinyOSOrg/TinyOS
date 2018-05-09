#ifndef DRIVE_DISK
#define DRIVE_DISK

#include<kernel/asm.h>
#include<shared/intdef.h>
#include<kernel/memory/phy_mem_man.h>
#include<shared/bool.h>

/*
    此头文件提供 write_disk、read_disk函数用于读写磁盘：
    {
         write_disk:每次从内存往磁盘传递一个扇区大小的数据（512字节）；
         read_disk：从磁盘往内存一个固定位置传递一个扇区大小的数据（512字节）；
         由调用者保证调用write_disk和read_disk时不会发生缺页中断,保证二者在使用时不会发生
         越界问题；
         读写磁盘使用LBA28模式，primary通道；
    }
*/

bool read_disk(uint32_t disk, uint16_t* main_memory);

bool write_disk(uint16_t* main_memory, uint32_t disk);

#endif
