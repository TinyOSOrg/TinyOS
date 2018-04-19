#include <kernel/assert.h>
#include <kernel/interrupt.h>
#include <kernel/kbdriver.h>
#include <kernel/process/process.h>
#include <kernel/sysmsg/sysmsg_src.h>

#define KB_PRESSED_BITMAP_COUNT (256 / 32)

static struct sysmsg_receiver_list kb_receivers;

/* 记录按键是否被按下的位图 */
static uint32_t key_pressed[KB_PRESSED_BITMAP_COUNT];

/* 初始化键盘控制芯片 */
static void init_8042(void)
{

}

/* 键盘中断处理 */
static void kb_intr_handler(void)
{
    
}

void init_kb_driver(void)
{
    init_8042();

    init_sysmsg_receiver_list(&kb_receivers);
    
    for(size_t i = 0;i != KB_PRESSED_BITMAP_COUNT; ++i)
        key_pressed[i] = 0x00000000;
        
    set_intr_function(INTR_NUMBER_KEYBOARD, kb_intr_handler);
}

void subscribe_kb(struct PCB *pcb)
{
    ASSERT_S(pcb);
    register_sysmsg_source(pcb, &kb_receivers, &pcb->sys_msg_srcs);
}

bool is_key_pressed(uint8_t keycode)
{
    return (key_pressed[keycode >> 5] & (1 << (keycode &0x1f))) != 0;
}
