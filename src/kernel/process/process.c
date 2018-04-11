#include <kernel/asm.h>
#include <kernel/boot.h>
#include <kernel/process/process.h>
#include <kernel/process/thread.h>
#include <kernel/seg_desc.h>

#include <lib/intdef.h>
#include <lib/string.h>

struct TSS
{
    uint32_t BL;
    uint32_t esp0, ss0;
    uint32_t esp1, ss1;
    uint32_t esp2, ss2;
    uint32_t cr3;
    uint32_t eip;
    uint32_t eflags;
    uint32_t eax, ecx, edx, ebx;
    uint32_t esp, ebp, esi, edi;
    uint32_t es, cs, ss, ds, fs, gs;
    uint32_t LDT;
    uint32_t trace;
    uint32_t IO;
};

static struct TSS tss;

/* 初始化TSS描述符 */
static void init_TSS(void)
{
    memset((char*)&tss, 0x0, sizeof(struct TSS));
    tss.ss0 = SEG_SEL_KERNEL_STACK;
    tss.IO = sizeof(struct TSS);
    make_iGDT((struct iGDT*)((char*)GDT_START + 3 * 8),
              (uint32_t)&tss, sizeof(struct TSS) - 1, 
              TSS_ATTRIB_LOW, TSS_ATTRIB_HIGH);
}

/* 初始化用户代码段和数据段描述符 */
static void init_user_segments(void)
{
    make_iGDT((struct iGDT*)((char*)GDT_START + 4 * 8),
              0, 0xfffff,
              GDT_ATTRIB_LOW_CODE, GDT_ATTRIB_HIGH);
    make_iGDT((struct iGDT*)((char*)GDT_START + 5 * 8),
              0, 0xfffff,
              GDT_ATTRIB_LOW_DATA, GDT_ATTRIB_HIGH);
}

void init_process_man(void)
{
    init_TSS();
    init_user_segments();

    _load_GDT((uint32_t)GDT_START, 8 * 6 - 1);
    _ltr(TSS_SEL);
}
