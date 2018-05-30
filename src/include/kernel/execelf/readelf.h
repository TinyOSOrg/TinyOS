#ifndef TINY_OS_READELF_H
#define TINY_OS_READELF_H

#include <shared/stdint.h>

/*
    装载一个32位elf程序
    elf_addr为文件内容在内存中的地址
    返回值为程序入口指针

    beg和end用来返回elf中程序所占区域[beg, end)
*/
void *load_elf(const void *elf_addr, size_t *beg, size_t *end);

#endif /* TINY_OS_READELF_H */
