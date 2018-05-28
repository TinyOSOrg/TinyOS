#include <kernel/execelf/readelf.h>

#include <shared/intdef.h>

#include <lib/string.h>
#include <lib/sys.h>

/*
    IMPROVE：load_elf应该进行适当的合法性检查
*/

#define PT_LOAD 1

#define EI_NIDENT 16

#define ELF32_Addr  uint32_t
#define ELF32_Half  uint16_t
#define ELF32_Off   uint32_t 
#define ELF32_SWord int32_t
#define ELF32_Word  uint32_t

typedef struct {
    unsigned char e_ident[EI_NIDENT];
    ELF32_Half e_type;
    ELF32_Half e_machine;
    ELF32_Word e_version;
    ELF32_Addr e_entry;     // virtual address program entry
    ELF32_Off  e_phoff;     // program header offset
    ELF32_Off  e_shoff;
    ELF32_Word e_flags;
    ELF32_Half e_ehsize;    // elf header size
    ELF32_Half e_phentsize; // program header size
    ELF32_Half e_phnum;     // program header numbers
    ELF32_Half e_shentsize;
    ELF32_Half e_shnum;
    ELF32_Half e_shstrndx;
} elf32_ehdr;

typedef struct {
    ELF32_Word p_type;     // Only PT_LOAD need loading
    ELF32_Off  p_offset;
    ELF32_Addr p_vaddr;
    ELF32_Addr p_paddr;
    ELF32_Word p_filesz;
    ELF32_Word p_memsz;
    ELF32_Word p_flags;
    ELF32_Word p_align;
} elf32_phdr;

void *load_elf(const void *_filestart)
{
    elf32_ehdr eh;
    const char *filestart = (const char *)_filestart;

    int pos = 0;
    for (pos = 0; pos < EI_NIDENT; pos++)
        eh.e_ident[pos] = *(filestart + pos);
    
    //initialize ELF header

    eh.e_type      = *(ELF32_Half *)(filestart + pos); pos += sizeof(ELF32_Half);
    eh.e_machine   = *(ELF32_Half *)(filestart + pos); pos += sizeof(ELF32_Half);
    eh.e_version   = *(ELF32_Word *)(filestart + pos); pos += sizeof(ELF32_Word);
    eh.e_entry     = *(ELF32_Addr *)(filestart + pos); pos += sizeof(ELF32_Addr);
    eh.e_phoff     = *(ELF32_Off  *)(filestart + pos); pos += sizeof(ELF32_Off);
    eh.e_shoff     = *(ELF32_Off  *)(filestart + pos); pos += sizeof(ELF32_Off);
    eh.e_flags     = *(ELF32_Word *)(filestart + pos); pos += sizeof(ELF32_Word);
    eh.e_ehsize    = *(ELF32_Half *)(filestart + pos); pos += sizeof(ELF32_Half);
    eh.e_phentsize = *(ELF32_Half *)(filestart + pos); pos += sizeof(ELF32_Half);
    eh.e_phnum     = *(ELF32_Half *)(filestart + pos); pos += sizeof(ELF32_Half);
    eh.e_shentsize = *(ELF32_Half *)(filestart + pos); pos += sizeof(ELF32_Half);
    eh.e_shnum     = *(ELF32_Half *)(filestart + pos); pos += sizeof(ELF32_Half);
    eh.e_shstrndx  = *(ELF32_Half *)(filestart + pos); pos += sizeof(ELF32_Half);

    elf32_phdr header;
    pos = eh.e_phoff;
    for(uint16_t i = 0; i < eh.e_phnum; i++)
    {
        header.p_type   = *(ELF32_Word *)(filestart + pos); pos += sizeof(ELF32_Word);
	    header.p_offset = *(ELF32_Off  *)(filestart + pos); pos += sizeof(ELF32_Off);
	    header.p_vaddr  = *(ELF32_Addr *)(filestart + pos); pos += sizeof(ELF32_Addr);
	    header.p_paddr  = *(ELF32_Addr *)(filestart + pos); pos += sizeof(ELF32_Addr);
	    header.p_filesz = *(ELF32_Word *)(filestart + pos); pos += sizeof(ELF32_Word);
	    header.p_memsz  = *(ELF32_Word *)(filestart + pos); pos += sizeof(ELF32_Word);
	    header.p_flags  = *(ELF32_Word *)(filestart + pos); pos += sizeof(ELF32_Word);
	    header.p_align  = *(ELF32_Word *)(filestart + pos); pos += sizeof(ELF32_Word);

        if(header.p_type != PT_LOAD)
            continue;

        memcpy((char*)header.p_vaddr, (const char *)(filestart + header.p_offset),
               header.p_filesz);
        if(header.p_memsz > header.p_filesz)
        {
            memset((char *)((char*)header.p_vaddr + header.p_filesz), 0x0,
                   header.p_memsz - header.p_filesz);
        }
    }

    return (void*)eh.e_entry;
}
