#ifndef TINY_OS_LIB_FSTM_H
#define TINY_OS_LIB_FSTM_H

#include <shared/sys.h>

#define FSTM_BUF_SIZE_DEFAULT 128

typedef struct
{
    // 文件指针
    usr_file_handle fp;
    // 文件中的下一个读取位置
    uint32_t fpos;
    // 文件大小
    uint32_t fsize;
    // 缓冲区大小
    size_t bsize;
    // 缓冲区内的下一个读取位置
    size_t bidx;
    // 缓冲区内的有效数据区大小
    size_t dsize;
    // 缓冲区入口，gcc扩展
    uint8_t buf[0];
} ifstm_t;

#define FSTM_EOF ((char)0xff)

/* 创建一个输入文件流 */
ifstm_t *ifstm_init(filesys_dp_handle dp, const char *path,
                    size_t buf_size);

/* 读输入文件流中的下一个字符 */
char ifstm_next(ifstm_t *fstm);

/* 销毁一个输入文件流 */
void ifstm_free(ifstm_t *fstm);

/* 输入文件流中还有多少字符 */
size_t ifstm_remain_size(ifstm_t *fstm);

#endif /* TINY_OS_LIB_FSTM_H */
