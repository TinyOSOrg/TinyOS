#ifndef TINY_OS_READELF_H
#define TINY_OS_READELF_H

/*class Elfloader
{
public:
        Elfloader(const char *);
        bool load_elf();
        void testout();//need deleted
private:
        ELF32_Addr Base;
        Elf32_Ehdr Elfheader;
        Elf32_Phdr * Programheader;
        char * filestart;
        void load_ph(ELF32_Addr address);
        //ool map_elf();//link file adress with virtual adress.
};*/

void *load_elf(const void *elf_addr);

#endif /* TINY_OS_READELF_H */
