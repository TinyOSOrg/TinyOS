#include <kernel/asm.h>
#include <kernel/assert.h>
#include <kernel/interrupt.h>
#include <kernel/kbdriver.h>
#include <kernel/process/process.h>
#include <kernel/sysmsg/sysmsg_src.h>

#include <lib/keycode.h>

/* 按键位图数量 */
#define KB_PRESSED_BITMAP_COUNT (256 / 32)

#define KB_8042_OUT_BUF 0x60 /* 输出缓冲 */
#define KB_8042_IN_BUF  0x60 /* 输入缓冲 */
#define KB_8042_STS_REG 0x64 /* 状态寄存器 */
#define KB_8042_CTL_REG 0x64 /* 控制寄存器 */

/* 某个扫描码如何被对应到VK值以及字符 */
struct scancode_trans_unit
{
    uint8_t vk;
    char ch;
    char upch;
};

#define CHAR_NULL '\0'

/* 访问方式：scancode_translator[通码 - 1] */
static struct scancode_trans_unit scancode_translator[] =
{
    { VK_ESCAPE,  CHAR_NULL, CHAR_NULL },
    { '1',        '1',       '!'       },
    { '2',        '2',       '@'       },
    { '3',        '3',       '#'       },
    { '4',        '4',       '$'       },
    { '5',        '5',       '%'       },
    { '6',        '6',       '^'       },
    { '7',        '7',       '&'       },
    { '8',        '8',       '*'       },
    { '9',        '9',       '('       },
    { '0',        '0',       ')'       },
    { VK_MINUS,   '-',       '_'       },
    { VK_EQUAL,   '=',       '+'       },
    { VK_BS,      '\b',      '\b'      },
    { VK_TAB,     '\t',      '\t'      },
    { 'Q',        'q',       'Q'       },
    { 'W',        'w',       'W'       },
    { 'E',        'e',       'E'       },
    { 'R',        'r',       'R'       },
    { 'T',        't',       'T'       },
    { 'Y',        'y',       'Y'       },
    { 'U',        'u',       'U'       },
    { 'I',        'i',       'I'       },
    { 'O',        'o',       'O'       },
    { 'P',        'p',       'P'       },
    { VK_LBRAC,   '[',       '{'       },
    { VK_RBRAC,   ']',       '}'       },
    { VK_ENTER,   '\n',      '\n'      },
    { VK_LCTRL,   CHAR_NULL, CHAR_NULL },
    { 'A',        'a',       'A'       },
    { 'S',        's',       'S'       },
    { 'D',        'd',       'D'       },
    { 'F',        'f',       'F'       },
    { 'G',        'g',       'G'       },
    { 'H',        'h',       'H'       },
    { 'J',        'j',       'J'       },
    { 'K',        'k',       'K'       },
    { 'L',        'l',       'L'       },
    { VK_SEMICOL, ';',       ':'       },
    { VK_QOT,     '\'',      '\"'      },
    { VK_SIM,     '`',       '~'       },
    { VK_LSHIFT,  CHAR_NULL, CHAR_NULL },
    { VK_BACKSL,  '\\',      '|'       },
    { 'Z',        'z',       'Z'       },
    { 'X',        'x',       'X'       },
    { 'C',        'c',       'C'       },
    { 'V',        'v',       'V'       },
    { 'B',        'b',       'B'       },
    { 'N',        'n',       'N'       },
    { 'M',        'm',       'M'       },
    { VK_COMMA,   ',',       '<'       },
    { VK_POINT,   '.',       '>'       },
    { VK_DIV,     '/',       '?'       },
    { VK_RSHIFT,  CHAR_NULL, CHAR_NULL },
    { VK_NULL,    CHAR_NULL, CHAR_NULL },
    { VK_LALT,    CHAR_NULL, CHAR_NULL },
    { VK_SPACE,   ' ',       ' '       },
    { VK_CAPS,    CHAR_NULL, CHAR_NULL },
    { VK_F1,      CHAR_NULL, CHAR_NULL },
    { VK_F2,      CHAR_NULL, CHAR_NULL },
    { VK_F3,      CHAR_NULL, CHAR_NULL },
    { VK_F4,      CHAR_NULL, CHAR_NULL },
    { VK_F5,      CHAR_NULL, CHAR_NULL },
    { VK_F6,      CHAR_NULL, CHAR_NULL },
    { VK_F7,      CHAR_NULL, CHAR_NULL },
    { VK_F8,      CHAR_NULL, CHAR_NULL },
    { VK_F9,      CHAR_NULL, CHAR_NULL },
    { VK_F10,     CHAR_NULL, CHAR_NULL },
    /* F11和F12的扫描码并不跟在F10后面，懒得支持了…… */
    /* RCTRL和RALT都是e0打头，这两个键单独判断 */
};

/* 按键消息接收者队列 */
static struct sysmsg_receiver_list kb_receivers;

/* 字符消息接收者队列 */
static struct sysmsg_receiver_list char_receivers;

/* 记录按键是否被按下的位图 */
static uint32_t key_pressed[KB_PRESSED_BITMAP_COUNT];

/* 是否来了个e0码 */
static bool scancode_e0;

/* 大写是否开启 */
static bool caps_lock;

static void set_key_pressed(uint8_t vk, bool pressed)
{
    uint8_t idx = (vk >> 5), off = (vk & 0x1f);
    if(pressed)
        key_pressed[idx] |= (1 << off);
    else
        key_pressed[idx] &= ~(1 << off);
}

/* 键盘中断处理 */
static void kb_intr_handler(void)
{
    bool    up = false;
    uint8_t vk = VK_NULL;
    char    ch = CHAR_NULL;

    uint8_t sc = _in_byte_from_port(KB_8042_IN_BUF);

    if(sc == 0xe0)
    {
        scancode_e0 = true;
        return;
    }
    else if(scancode_e0)
    {
        scancode_e0 = false;
        if(sc == 0x1d) // R CTRL down
            vk = VK_RCTRL;
        else if(sc == 0x38) // R ALT down
            vk = VK_RALT;
        else if(sc == 0x9d) // R CTRL up
        {
            vk = VK_RCTRL;
            up = true;
        }
        else if(sc == 0xb8) // R ALT up
        {
            vk = VK_RALT;
            up = true;
        }
    }
    else
    {
        if(sc >= 0x80)
        {
            up = true;
            sc -= 0x80;
        }

        if(sc < 1 || sc >= ARRAY_SIZE(scancode_translator))
            return;

        uint32_t idx = sc - 1;
        vk = scancode_translator[idx].vk;
        if((is_key_pressed(VK_LSHIFT) || is_key_pressed(VK_RSHIFT)) ^ caps_lock)
            ch = scancode_translator[idx].upch;
        else
            ch = scancode_translator[idx].ch;
    }

    // 大写锁定
    if(vk == VK_CAPS && !up)
        caps_lock = !caps_lock;

    // 按键消息发布
    if(vk != VK_NULL)
    {
        set_key_pressed(vk, !up);

        struct kbmsg_struct kbmsg;
        kbmsg.type = SYSMSG_TYPE_KEYBOARD;
        kbmsg.key  = vk;
        kbmsg.flags = up ? 1 : 0;
        for(struct ilist_node *node = kb_receivers.processes.next;
            node != &kb_receivers.processes; node = node->next)
        {
            struct PCB *pcb = GET_STRUCT_FROM_MEMBER(struct sysmsg_rcv_src_list_node,
                                                     rcv_node, node)->pcb;
            send_sysmsg(&pcb->sys_msgs, (struct sysmsg*)&kbmsg);
        }
    }

    // 字符消息发布
    if(ch != CHAR_NULL && !up)
    {
        struct kbchar_msg_struct msg;
        msg.type = SYSMSG_TYPE_CHAR;
        msg.ch = ch;
        for(struct ilist_node *node = char_receivers.processes.next;
            node != &char_receivers.processes; node = node->next)
        {
            struct PCB *pcb = GET_STRUCT_FROM_MEMBER(struct sysmsg_rcv_src_list_node,
                                                     rcv_node, node)->pcb;
            send_sysmsg(&pcb->sys_msgs, (struct sysmsg*)&msg);
        }
    }
}

void init_kb_driver(void)
{
    init_sysmsg_receiver_list(&kb_receivers);
    init_sysmsg_receiver_list(&char_receivers);
    
    for(size_t i = 0;i != KB_PRESSED_BITMAP_COUNT; ++i)
        key_pressed[i] = 0x00000000;
    
    scancode_e0 = false;
    caps_lock   = false;
        
    set_intr_function(INTR_NUMBER_KEYBOARD, kb_intr_handler);
}

void subscribe_kb(struct PCB *pcb)
{
    ASSERT_S(pcb);
    register_sysmsg_source(pcb, &kb_receivers, &pcb->sys_msg_srcs);
}

void subscribe_char(struct PCB *pcb)
{
    ASSERT_S(pcb);
    register_sysmsg_source(pcb, &char_receivers, &pcb->sys_msg_srcs);
}

bool is_key_pressed(uint8_t keycode)
{
    return (key_pressed[keycode >> 5] & (1 << (keycode &0x1f))) != 0;
}

uint32_t syscall_keyboard_query_impl(uint32_t func, uint32_t arg)
{
    switch(func)
    {
    case KEYBOARD_SYSCALL_FUNCTION_IS_KEY_PRESSED:
        if(arg < ARRAY_SIZE(scancode_translator))
            return is_key_pressed(arg);
        return false;
    }
    return false;
}
