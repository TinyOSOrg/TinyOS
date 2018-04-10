#include <kernel/asm.h>
#include <kernel/assert.h>
#include <kernel/interrupt.h>
#include <kernel/print.h>
#include <kernel/seg_desc.h>

#include <lib/string.h>

/* 中断描述符构成 */

#define INTR_DESC_PRESENT (1 << 7)

#define INTR_DESC_DPL_0 (0 << 5)
#define INTR_DESC_DPL_1 (1 << 5)
#define INTR_DESC_DPL_2 (2 << 5)
#define INTR_DESC_DPL_3 (3 << 5)

#define INTR_DESC_TYPE 0xe

#define INTR_DESC_ATTRIB(DPL) (INTR_DESC_PRESENT | DPL | INTR_DESC_TYPE)

#define PIC_M_CTRL 0x20 /* 主片控制端口 */
#define PIC_M_DATA 0x21 /* 主片数据端口 */
#define PIC_S_CTRL 0xa0 /* 从片控制端口 */
#define PIC_S_DATA 0xa1 /* 从片数据端口 */

struct intr_gate_desc
{
    uint16_t offset_low16;  // function在目标段内偏移量的低16位
    uint16_t seg_sel;       // 目标代码段选择子
    uint8_t zero_pad;       // 固定是0
    uint8_t attrib;         // 一些属性，由INTR_DESC_ATTRIB给出
    uint16_t offset_high16; // 偏移量高16位
};

/* 在intr_entry.s中定义 */
extern uint32_t intr_entry_table[IDT_DESC_COUNT];

/* 可注册的中断处理函数 */
void (*volatile intr_function[IDT_DESC_COUNT])(void);

static struct intr_gate_desc IDT[IDT_DESC_COUNT];

void default_intr_function(uint8_t intr_number)
{
    if(intr_number == 0x27 || intr_number == 0x2f)
        return;
    print_format("int vec: %u\n", intr_number);
    while(1);
}

/* 初始化8259A */
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

        intr_function[i] = (void(*)(void))default_intr_function;
    }

    init_8259A();

    uint64_t IDTarg = (sizeof(IDT) - 1) | ((uint64_t)((uint64_t)((uint32_t)IDT) << 16));
    _load_IDT(IDTarg);
}

void set_intr_function(uint8_t intr_number, void (*func)(void))
{
    ASSERT_S(intr_number < IDT_DESC_COUNT);
    if(!func)
        intr_function[intr_number] = (void(*)(void))default_intr_function;
    else
        intr_function[intr_number] = func;
}

void set_8253_freq(uint16_t freq)
{
    uint16_t value = 1193180 / freq;

    _out_byte_to_port(0x43, (3 << 4) | (2 << 1));
    _out_byte_to_port(0x40, (uint8_t)value);
    _out_byte_to_port(0x40, (uint8_t)(value >> 8));
}

#define INTR_STATE_ON 0x1

bool is_intr_on(intr_state state)
{
    return state & INTR_STATE_ON != 0;
}

intr_state get_intr_state(void)
{
    return _get_eflag() & 0x00000200 ? INTR_STATE_ON : 0;
}

void set_intr_state(intr_state state)
{
    if(state & INTR_STATE_ON)
        _enable_intr();
    else
        _disable_intr();
}
