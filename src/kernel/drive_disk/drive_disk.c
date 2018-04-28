#include<kernel/asm.h>
#include<shared/intdef.h>
#include<kernel/memory/phy_mem_man.h>
#include<shared/bool.h>
#include<drive_disk/drive_disk.h>
#include<kernel/console/print.h>
/*
    set_buffer_addr函数仅在main.c函数开始时被调用一次;
    is_busy、is_wrong和one_sec_data_carry为内部函数,不要在其他文件调用此函数。
*/
uint16_t* buffer_addr;
void one_sec_data_carry(uint16_t* src, uint16_t* dst);
bool is_busy(void);
bool is_wrong(void);

void set_buffer_addr(void)
{
    buffer_addr = (uint16_t*)alloc_static_kernel_mem(512, 2);
}


bool missing_page_write_disk(uint16_t* main_memory, uint32_t disk)
{
    uint8_t low_8bits, mid_8bits, high_8bits, highest_8bits, cnt_sec = 1;
    low_8bits = disk;
    mid_8bits = disk >> 8;
    high_8bits = disk >> 16;
    highest_8bits = ((disk >> 24) & 0x0f) | 0xe0;  //除了地址最高的4位，还要设置写的模式为LBA，使用主盘。
    //关中断
    _disable_intr();
    //设置要写的扇区数为1
    _out_byte_to_port(0x1f2, cnt_sec);
    //设置写的地址
    _out_byte_to_port(0x1f3, low_8bits);
    _out_byte_to_port(0x1f4, mid_8bits);
    _out_byte_to_port(0x1f5, high_8bits);
    _out_byte_to_port(0x1f6, highest_8bits);
    //command:请求硬盘写
    _out_byte_to_port(0x1f7, 0x30);
    //判断硬盘控制器是否已准备好
    while(is_busy())
    {
        ;
    }
    //若出错，则返回flase
    if(is_wrong())
    {
        return false;
    }
    //开始写数据，每次写16个位，连续写256次，即每次调用此函数写512字节（一个扇区大小的数据量）。
    for(uint32_t i = 0; i < 256; ++i)
    {
        _out_double_byte_to_port(0x1f0, *main_memory++);
    }
    //开中断
    _enable_intr();
    return true;    
}

bool missing_page_read_disk(uint32_t disk, uint16_t* main_memory)
{
    uint8_t low_8bits, mid_8bits, high_8bits, highest_8bits, cnt_sec = 1;
    low_8bits = disk;
    mid_8bits = disk >> 8;
    high_8bits = disk >> 16;
    highest_8bits = ((disk >> 24) & 0x0f )| 0xe0;  //除了地址最高的4位，还要设置读取的模式为LBA，使用主盘。
     //关中断
    _disable_intr();
    //设置要读的扇区数为1
    _out_byte_to_port(0x1f2, cnt_sec);
    //设置读的地址
    _out_byte_to_port(0x1f3, low_8bits);
    _out_byte_to_port(0x1f4, mid_8bits);
    _out_byte_to_port(0x1f5, high_8bits);
    _out_byte_to_port(0x1f6, highest_8bits);
    //command:请求硬盘读
    _out_byte_to_port(0x1f7, 0x20);
    //判断硬盘是否准备好
    while(is_busy())
    {
        ;
    }
    //若出错，则返回flase
    if(is_wrong())
    {
        return false;
    }
    //开始读取数据，每次读取16位，连续读256次，即每次调用此函数读取512字节（一个扇区的数据量）。
    for(uint32_t i = 0; i < 256; ++i)
    { 
        *main_memory++ = _in_double_byte_from_port(0x1f0);
    }
    //开中断
    _enable_intr();
    return true;  
}

bool normal_read_disk(uint32_t disk, uint16_t* main_memory)
{
    if(missing_page_read_disk(disk, buffer_addr) == false)
        return false;
    one_sec_data_carry(buffer_addr, main_memory);
    return true;
}

bool normal_write_disk(uint16_t* main_memory, uint32_t disk)
{
    one_sec_data_carry(main_memory, buffer_addr);  
    if(missing_page_write_disk(buffer_addr, disk) == false)
        return false;  
    return true;
}

bool is_busy(void)
{
    uint8_t data_from_0x1f7, intermediate_variable;
    data_from_0x1f7 = _in_byte_from_port(0x1f7);
    intermediate_variable = data_from_0x1f7 & 0x08;
    if(intermediate_variable == 0x08)
        return false;
    else   
        return true; 
}

bool is_wrong(void)
{
    uint8_t stat;
    stat = _in_byte_from_port(0x1f7);
    if(stat & 0x80)
        return true;
    else
        return false;
}

void one_sec_data_carry(uint16_t* src, uint16_t* dst)
{
    for(size_t i = 0; i < 256; ++i)
    {
        *dst++ = *src++;
    }
}

void init_disk_drive(void)
{
    set_buffer_addr();
}