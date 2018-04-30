#ifndef FS_EVEI
#define FS_EVEI

#include<shared/intdef.h>
#include<shared/bool.h>
/*
    ext_variant_essential_info:ext文件系统变种的基本信息
*/
#define BLOCK_SIZE 512 //bytes
#define SECTION_SIZE 512 //bytes
#define INODE_RATIO 16 //代表每多少块就分配一个inode
#define SCALE (BLOCK_SIZE/SECTION_SIZE) //每块代表多少扇区

typedef char DataBlock[BLOCK_SIZE];

typedef struct _time_stamp{
    uint32_t year;
    uint8_t month;
    uint8_t day;
}TimeStamp; //6个字节

typedef struct _inode{
    uint32_t inum;   //inode节点编号
    uint8_t file_type;
    uint8_t authority;
    uint8_t owner;  //ID
    uint32_t file_size;
    TimeStamp time_stamp;
    uint32_t first_block_ptr[15];  //0~11为直接寻址指针，12为一级间接寻址指针，13为二级间接寻址指针，14为三级级间接寻址指针。
}Inode;  //77个字节

typedef struct _super_block{
    char fs_name[16];  //占16个字节，要有结束标志
    uint32_t fs_begin;  //文件系统的起始LBA28扇区号
    uint32_t fs_end;    //文件系统的终止LBA28扇区号
    TimeStamp fs_time_stamp;  //文件系统的时间戳
    uint32_t inode_num;       //inode的数量
    uint32_t data_block_num;  //data_block的数量
    uint32_t inode_idle;   //空闲的inode节点的数量
    uint32_t data_block_idle; //空闲的数据块的数量
    uint32_t inode_bmap_begin; //inode位图起始扇区的LBA28号
    uint32_t inode_bmap_end;  //inode位图截止扇区的LBA28号
    uint32_t inode_table_begin;  //inode表的起始扇区的LBA28号
    uint32_t inode_table_end;  //inode表的终止扇区的LBA28号
    uint32_t data_block_bmap_begin; //数据块位图起始扇区的LBA28号
    uint32_t data_block_bmap_end;   //数据块位图终止扇区的LBA28号
    uint32_t data_block_begin;  //数据块起始扇区的LBA28号
    uint32_t data_block_end;    //数据块终止扇区的LBA28号
}SuperBlock; //78个字节

#endif