#ifndef DRIVE_DISK
#define DRIVE_DISK

#include<kernel/asm.h>
#include<shared/intdef.h>
#include<kernel/memory/phy_mem_man.h>
#include<shared/bool.h>

/*
    此头文件提供 normal_write_disk、normal_read_disk、missing_page_write_disk、
    missing_page_read_disk函数用于读写磁盘：{
         normal_write_disk:每次从内存往磁盘传递一个扇区大小的数据（512字节）；
         normal_read_disk：从磁盘往内存一个固定位置传递一个扇区大小的数据（512字节）；
         missing_page_write_disk、missing_page_read_disk：缺页中断中调用，每次传递一个扇区大小的数据（512字节）；
         其余内核读写部分不应跳过这两个函数，是软件层面访问磁盘的最底层函数；
         函数write_disk和read_disk执行成功返回true，失败返回false；
         函数不提供越界检查，由使用者自己保证数据不会越界；
         读写磁盘使用LBA28模式，primary通道；
*/

//set_buffer_addr函数仅在main.c函数开始时被调用一次
void set_buffer_addr(void);

bool missing_page_write_disk(uint16_t* main_memory, uint32_t disk);

bool missing_page_read_disk(uint32_t disk, uint16_t* main_memory);

bool normal_read_disk(uint32_t disk, uint16_t* main_memory);

bool normal_write_disk(uint16_t* main_memory, uint32_t disk);

void init_disk_drive(void);
#endif
