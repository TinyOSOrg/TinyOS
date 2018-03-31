#include <kernel/asm.h>
#include <kernel/intr_entry.h>
#include <kernel/print.h>
#include <kernel/seg_desc.h>

// 中断描述符构成

#define INTR_DESC_PRESENT (1 << 7)

#define INTR_DESC_DPL_0 (0 << 5)
#define INTR_DESC_DPL_1 (1 << 5)
#define INTR_DESC_DPL_2 (2 << 5)
#define INTR_DESC_DPL_3 (3 << 5)

#define INTR_DESC_TYPE 0xe

#define INTR_DESC_ATTRIB(DPL) (INTR_DESC_PRESENT | DPL | INTR_DESC_TYPE)

#define PIC_M_CTRL 0x20 // 主片控制端口
#define PIC_M_DATA 0x21 // 主片数据端口
#define PIC_S_CTRL 0xa0 // 从片控制端口
#define PIC_S_DATA 0xa1 // 从片数据端口

struct intr_gate_desc
{
    uint16_t offset_low16;  // function在目标段内偏移量的低16位
    uint16_t seg_sel;       // 目标代码段选择子
    uint8_t zero_pad;       // 固定是0
    uint8_t attrib;         // 一些属性，由INTR_DESC_ATTRIB给出
    uint16_t offset_high16; // 偏移量高16位
};

#define IDT_DESC_COUNT 0x21

extern uint32_t intr_entry_table[IDT_DESC_COUNT]; //在intr_entry.s中定义

static struct intr_gate_desc IDT[IDT_DESC_COUNT];

// 初始化8259A
static void init_8259A(void)
{
   // 主片
   _out_byte_to_port(PIC_M_CTRL, 0x11);
   _out_byte_to_port(PIC_M_DATA, 0x20);
   _out_byte_to_port(PIC_M_DATA, 0x04);
   _out_byte_to_port(PIC_M_DATA, 0x01);

   // 从片
   _out_byte_to_port(PIC_S_CTRL, 0x11);
   _out_byte_to_port(PIC_S_DATA, 0x28);
   _out_byte_to_port(PIC_S_DATA, 0x02);
   _out_byte_to_port(PIC_S_DATA, 0x01);

   // 打开IR0
   _out_byte_to_port(PIC_M_DATA, 0xfe);
   _out_byte_to_port(PIC_S_DATA, 0xff);
}

void init_IDT(void)
{
    for(int i = 0;i < IDT_DESC_COUNT; ++i)
    {
        struct intr_gate_desc *desc = &IDT[i];
        desc->offset_low16  = intr_entry_table[i] & 0xffff;
        desc->seg_sel       = SEG_SEL_KERNEL_CODE;
        desc->zero_pad      = 0;
        desc->attrib        = INTR_DESC_ATTRIB(INTR_DESC_DPL_0);
        desc->offset_high16 = intr_entry_table[i] >> 16;
    }
    _put_str("IDT desc initialized\n");

    init_8259A();
    _put_str("8259A initialized\n");

    uint64_t IDTarg = (sizeof(IDT) - 1) | ((uint64_t)((uint32_t)IDT << 16));
    asm volatile ("lidt %0" : : "m" (IDTarg));
    _put_str("IDT initialized\n");
}
