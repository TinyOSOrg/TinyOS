#include<kernel/fs/fs_start.h>
#include<shared/intdef.h>
#include<shared/bool.h>
#include<drive_disk/drive_disk.h>
#include<kernel/memory/vir_mem_man.h>
#include<kernel/fs/fs_evei.h>
//只有read_partition_table函数可以修改partition_table_item_ptr变量的值
static PartitionTableItem* partition_table_item_ptr;
static void read_partition_table(void);

void formatting_fs(PartitionTableItem* item_ptr)
{
    //开始进行格式化文件系统的操作

}

void init_fs(void)
{
    //读取分区表
    read_partition_table();
    PartitionTableItem* temp_item_ptr = partition_table_item_ptr;
    for(uint32_t i = 0; i < 8; ++i)
    {
        if(temp_item_ptr->is_null == 1) 
            ;
        else
        {
            if(temp_item_ptr->is_formatting == 1)
                ;
            else
            {
                //根据temp_item_ptr执行formatting_fs函数
            }
        }
        temp_item_ptr++;
    }
    //进行其它的初始化操作
}
 
static void read_partition_table(void)
{
    partition_table_item_ptr = (PartitionTableItem*)alloc_ker_page(true);
    normal_read_disk(PARTITION_TABLE_SECTION,(uint16_t*)partition_table_item_ptr);
}

void get_partition_table_info(PartitionTableInfo is_null)
{
    PartitionTableItem* temp_item_ptr = partition_table_item_ptr;
    for(uint32_t i = 0; i < 8; ++i)
    {
        if(temp_item_ptr->is_null == 1)
        {
            is_null[i] = true;
        }
        else
        {
            is_null[i] = false;
        }
    }
}

//item_ptr为所要更改的分区表项的编号，值为０～７；item_ptr指向在函数之外填充好的PartitionTableItem对象
void write_partition_table_item(uint32_t item_num, PartitionTableItem* item_ptr)
{
    //首先更改内存中的磁盘映像
    partition_table_item_ptr[item_num] = *item_ptr;
    //在更改磁盘
    normal_write_disk((uint16_t*)partition_table_item_ptr, PARTITION_TABLE_SECTION);
}