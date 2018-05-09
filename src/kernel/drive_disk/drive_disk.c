#include<kernel/asm.h>
#include<shared/intdef.h>
#include<kernel/memory/phy_mem_man.h>
#include<shared/bool.h>
#include<kernel/console/print.h>

/*
    任务队列；
    检查硬盘是否正忙：
    {
        如果忙，将自己添加到队列中并阻塞自己；
        如果不忙，将相关控制命令提交给磁盘，之后阻塞自己，等待中断唤醒自己；
    }
    传输数据完成后，检查阻塞队列：
    {
        若阻塞队列不为空，则唤醒阻塞队列队头的一个线程，否则read_disk或write_disk函数
        完成。
    }
    注意事项：
        磁盘读写必须是互斥的；
    }
    问题：
    {
        为什么是线程而不是进程阻塞？
        磁盘完成后会自动产生相应的中断号？
        为什么是循环链表？侵入式链表？
        如何将现在的线程添加到阻塞队列？
        如何将现在的线程从阻塞队列唤醒？
        如何检查磁盘当前是否可以传输数据？
    }
*/

/*   
    is_wrong为内部函数,不要在其他文件调用此函数。
*/
static bool is_wrong(void);

bool write_disk(uint16_t* main_memory, uint32_t disk)
{
    //首先检查硬盘是否正忙
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
    //阻塞自己等待中断
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

bool read_disk(uint32_t disk, uint16_t* main_memory)
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
    //阻塞自己等待硬盘准备好

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

static bool is_wrong(void)
{
    uint8_t stat;
    stat = _in_byte_from_port(0x1f7);
    if(stat & 0x80)
        return true;
    else
        return false;
}
