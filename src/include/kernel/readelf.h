#ifndef TINY_OS_READELF_H
#define TINY_OS_READELF_H

/*
    装载一个32位elf程序
    elf_addr为文件内容在内存中的地址
    返回值为程序入口指针
*/
void *load_elf(const void *elf_addr);

#endif /* TINY_OS_READELF_H */
