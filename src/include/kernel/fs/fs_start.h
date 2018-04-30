#ifndef FORMATTING_H
#define FORMATTING_H 

#include<shared/intdef.h>
#include<shared/bool.h>
/*
    文件系统的分区表占用一个扇区（在LBA中编号为300的扇区），
    整个分区表在内存上是连续的，每个区表项占用64个字节,一共有8个分区表项，
    每个表项多余的空间留作未来备份，
    分区表所在扇区的LBA编号由宏PARTION_TABLE_SECTION确定。
*/
#define PARTITION_TABLE_SECTION 300
#define PARTITION_STATE_ITEM_EMPTY -2
#define PARTITION_STATE_NONEMPTY_NOT_FORMATTING -1
#define PARTITION_STATE_NONEMPTY_AND_FORMATTING 0

typedef struct _partition_table_item{
    int32_t partition_state; //-2表示分区表项为空，-1表示分区表项非空但分区未格式化，0表示分区表项非空且分区已格式化
    uint32_t fs_type;  //用数字表示文件系统的类型：0表示ext_variant
    char fs_name[16];  //分区的名称，可由用户更改，占16个字节，要有结束标志，结束标志使用'\0'
    uint32_t fs_begin;
    uint32_t fs_end;
    //还剩32个字节留作备用
    char reserve[32];
}PartitionTableItem;

typedef bool PartitionTableInfo[8]; //用于返回每个表项是否为空，若某项为空，则相应值置为

void init_fs(void);
//函数使用者定义PartitionTableInfo类型的变量并传入函数，函数会根据
//PartitionTable的情况设置数组的每一位，为true则表示相应分区表项
//为空，为false则表示相应分区表项已有定义。
void formatting_fs(PartitionTableItem* item_ptr); 
void get_partition_table_info(PartitionTableInfo is_null);
//item_ptr为所要更改的分区表项的编号，值为０～７；item_ptr指向在函数之外填充好的PartitionTableItem对象
void write_partition_table_item(uint32_t item_num, PartitionTableItem* item_ptr);
#endif
