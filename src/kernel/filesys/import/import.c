#include <kernel/driver/diskdriver.h>
#include <kernel/filesys/import/import.h>
#include <kernel/memory.h>

#include <shared/filesys/import/import.h>

#include <lib/sys.h>

#define IPT_SECTOR_BYTE_SIZE 512

/*
    导入过程是对import分区的一次线性扫描，但只占用一个扇区大小的内存空间
    因此，当扫描跨磁盘扇区时，这部分空间的内容要自动替换，扫描指针也要更新
    这些信息由ipt_context追踪
*/
struct ipt_context
{
    uint32_t cur_sec;
    uint32_t lcl_pos;
    uint8_t *data;
};

/* 开始import分区扫描 */
static void ipt_begin(struct ipt_context *ctx, uint32_t beg_sec)
{
    ctx->cur_sec = beg_sec;
    ctx->lcl_pos = 0;

    // 说是只有一个扇区，咱们还是只能申请一个页
    // 这是个设计失误，即文件系统没有提供一个统一的扇区级内存管理系统
    ctx->data    = (uint8_t*)alloc_ker_page(false);
    disk_read(beg_sec, 1, ctx->data);
}

/*
    取得import分区的下一个字节
    为什么只能一个字节一个字节地读呢？因为这样比较好写，而且import本来就不是核心功能……
*/
static uint8_t ipt_next(struct ipt_context *ctx)
{
    uint8_t ret = ctx->data[ctx->lcl_pos++];

    // 是否要更新缓存扇区
    if(ctx->lcl_pos >= IPT_SECTOR_BYTE_SIZE)
    {
        ctx->cur_sec++;
        ctx->lcl_pos = 0;
        disk_read(ctx->cur_sec, 1, ctx->data);
    }

    return ret;
}

/* 结束对import分区的扫描，释放临时空间 */
static void ipt_end(struct ipt_context *ctx)
{
    free_ker_page(ctx->data);
}

void ipt_import_from_dp(uint32_t dp_beg)
{
    struct ipt_context ctx;
    ipt_begin(&ctx, dp_beg);

    // 临时字节指针
    uint8_t *p;
#define PS(V) do { p = (uint8_t*)(V); } while(0)
#define PB()  do { *p++ = ipt_next(&ctx); } while(0)
#define PR(DST) \
    do { PS(DST); for(size_t i = 0; i < sizeof(*DST); ++i) PB(); } while(0)

    // 首先取得文件数量，连续读sizeof(uint32_t)个字节
    // x86是小端序，所以直接地址递增

    uint32_t file_count;
    PR(&file_count);

    // 一个一个读文件
    while(file_count-- > 0)
    {
        // 分区号
        uint32_t dp;
        PR(&dp);

        // 文件名
        char path[IPT_PATH_BUF_SIZE];
        PS(path);
        for(size_t i = 0; i < IPT_PATH_BUF_SIZE; ++i)
            PB();
        
        // 文件字节数
        uint32_t file_size;
        PR(&file_size);

        // 读文件内容并写入到磁盘上的文件系统
        
        usr_file_handle fp;
        remove_file(dp, path);
        make_file(dp, path);
        open_file(dp, path, true, &fp);
        
        for(uint32_t i = 0; i < file_size; ++i)
        {
            uint8_t b;
            PR(&b);
            write_file(fp, i, 1, &b);
        }

        close_file(fp);
    }

    ipt_end(&ctx);

#undef PS
#undef PB
#undef PR
}
