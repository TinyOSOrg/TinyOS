#include <kernel/asm.h>
#include <kernel/assert.h>
#include <kernel/interrupt.h>
#include <kernel/process/process.h>
#include <kernel/seg_desc.h>

#include <shared/string.h>

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
void (*volatile intr_function[IDT_DESC_COUNT])();

static struct intr_gate_desc IDT[IDT_DESC_COUNT];

void default_intr_function(uint32_t intr_number)
{
    if(intr_number == 0x27 || intr_number == 0x2f)
        return;
    char buf[40] = "unprocessed intr: ";
    char int_buf[30];
    uint32_to_str(intr_number, int_buf);
    strcat(buf, int_buf);
    strcat(buf, " from process ");
    strcat(buf, get_cur_PCB()->name);
    FATAL_ERROR(buf);
}

/* 初始化8259A */
static void init_8259A()
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

   // 打开IR0和IR1
   _out_byte_to_port(PIC_M_DATA, 0xf8);
   _out_byte_to_port(PIC_S_DATA, 0xbf);
}

void init_IDT()
{
    for(int i = 0;i < IDT_DESC_COUNT; ++i)
    {
        struct intr_gate_desc *desc = &IDT[i];
        desc->offset_low16  = intr_entry_table[i] & 0xffff;
        desc->seg_sel       = SEG_SEL_KERNEL_CODE;
        desc->zero_pad      = 0;
        desc->attrib        = INTR_DESC_ATTRIB(INTR_DESC_DPL_0);
        desc->offset_high16 = intr_entry_table[i] >> 16;

        intr_function[i] = (void(*)())default_intr_function;
    }

    // 初始化系统调用对应的IDT

    extern uint32_t global_syscall_entry();
    struct intr_gate_desc *syscall_desc = &IDT[INTR_NUMBER_SYSCALL];
    syscall_desc->offset_low16  = (uint32_t)&global_syscall_entry & 0xffff;
    syscall_desc->seg_sel       = SEG_SEL_KERNEL_CODE;
    syscall_desc->zero_pad      = 0;
    syscall_desc->attrib        = INTR_DESC_ATTRIB(INTR_DESC_DPL_3);
    syscall_desc->offset_high16 = (uint32_t)&global_syscall_entry >> 16;

    init_8259A();

    uint64_t IDTarg = (sizeof(IDT) - 1) | (((uint64_t)(uint32_t)IDT) << 16);
    _load_IDT(IDTarg);
}

void set_intr_function(uint8_t intr_number, void (*func)())
{
    ASSERT_S(intr_number < IDT_DESC_COUNT);
    if(!func)
        intr_function[intr_number] = (void(*)())default_intr_function;
    else
        intr_function[intr_number] = func;
}

void set_8253_freq(uint16_t freq)
{
    uint32_t value = ((1193182u + (freq / 2)) / freq) & 0xffff;

    _out_byte_to_port(0x43, 0x04 | 0x30);
    _out_byte_to_port(0x40, (uint8_t)(value & 0x00ff));
    _out_byte_to_port(0x40, (uint8_t)(value >> 8));
}

#define INTR_STATE_ON 0x1

bool is_intr_on(intr_state state)
{
    return (state & INTR_STATE_ON) != 0;
}

intr_state get_intr_state()
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

intr_state fetch_and_disable_intr()
{
    intr_state rt = get_intr_state();
    _disable_intr();
    return rt;
}

intr_state fetch_and_enable_intr()
{
    intr_state rt = get_intr_state();
    _enable_intr();
    return rt;
}
