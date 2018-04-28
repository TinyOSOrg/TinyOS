#ifndef FS_BASIC
#define FS_BASIC

#include<shared/intdef.h>
#include<shared/bool.h>
#define BLOCK_SIZE 512 //bytes
#define DISK_SIZE 128  //MBytes，DISK_SIZE为文件系统实际所占用的磁盘大小
#define INODE_RATIO 16 //代表每多少块就分配一个inode
#define SCALE 1        //每块代表多少扇区
#define OFFSET 300     //起始偏移扇区数
#define INODE_NUM (((DISK_SIZE*1024*1024)/BLOCK_SIZE)/INODE_RATIO) 
#define BLOCK_NUM ((DISK_SIZE*1024*1024)/BLOCK_SIZE)

typedef char DataBlock[BLOCK_SIZE];

typedef struct _time_stamp{
    int year;
    int month;
    int day;
}TimeStamp;

typedef struct _inode{
    uint32_t inum;
    int8_t file_type;
    int8_t authority;
    int owner;  //ID
    uint32_t file_size;
    TimeStamp time_stamp;
    uint32_t first_block_ptr[15];
}Inode;

typedef struct _super_block{
    TimeStamp fs_time_stamp;
    uint32_t block_idle;
    uint32_t inode_idle;
}SuperBlock;

typedef bool BlockBitMap[BLOCK_NUM];

typedef bool IBitMap[INODE_NUM];

#endif