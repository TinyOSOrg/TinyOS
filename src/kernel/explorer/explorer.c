#include <kernel/assert.h>
#include <kernel/console/console.h>
#include <kernel/explorer/explorer.h>
#include <kernel/explorer/screen.h>
#include <kernel/filesys/dpt.h>
#include <kernel/filesys/import/import.h>
#include <kernel/interrupt.h>
#include <kernel/kernel.h>
#include <kernel/memory.h>
#include <kernel/process/process.h>
#include <kernel/execelf/execelf.h>

#include <shared/string.h>

#include <lib/sys.h>

/* Explorer状态 */
enum explorer_state
{
    es_fg,  // explorer本身处于前台，可接受输入
    es_bg,  // 有接管了输入输出的进程处于前台
};

/*
    当前处于前台的进程
    为空表示explorer自身应被显示
*/
static struct PCB *cur_fg_proc;

/* explorer自身的进程PCB，永不失效（ */
static struct PCB *expl_proc;

/* explorer状态 */
static enum explorer_state expl_stat;

/* explorer命令输入缓冲大小，三行 */
#define EXPL_CMD_INPUT_BUF_SIZE (SCR_CMD_WIDTH * SCR_CMD_HEIGHT)

/* explorer命令输入缓冲 */
static char *expl_cmd_input_buf;
static uint32_t expl_cmd_input_size;

/* 将屏幕内容拷贝到一个进程的显示缓冲区 */
void copy_scr_to_con_buf(struct PCB *pcb)
{
    intr_state is = fetch_and_disable_intr();

    struct con_buf *disp = pcb->disp_buf;

    if(disp)
    {
        semaphore_wait(&get_sys_con_buf()->lock);
        semaphore_wait(&disp->lock);
        
        memcpy(disp->data, get_sys_con_buf()->data, CON_BUF_BYTE_SIZE);
        disp->cursor = get_sys_con_buf()->cursor;
        
        semaphore_signal(&disp->lock);
        semaphore_signal(&get_sys_con_buf()->lock);
    }

    set_intr_state(is);
}

/* 将一个进程后台缓冲区的内容拷贝到屏幕 */
void copy_con_buf_to_scr(struct PCB *pcb)
{
    intr_state is = fetch_and_disable_intr();

    struct con_buf *disp = pcb->disp_buf;

    if(disp)
    {
        semaphore_wait(&get_sys_con_buf()->lock);
        semaphore_wait(&disp->lock);
        
        memcpy(get_sys_con_buf()->data, disp->data, CON_BUF_BYTE_SIZE);
        get_sys_con_buf()->cursor = disp->cursor;
        
        semaphore_signal(&disp->lock);
        semaphore_signal(&get_sys_con_buf()->lock);
    }

    set_intr_state(is);
}

static void init_explorer()
{
    intr_state is = fetch_and_disable_intr();

    // explorer进程永远处于“前台”，以遍接收键盘消息
    // 话虽如此，并不能随便输出东西到系统控制台上

    struct PCB *pcb = get_cur_PCB();

    // explorer应该是1号进程
    ASSERT_S(pcb->pid == 1);

    // 把自己从PCB列表中摘除
    erase_from_ilist(&pcb->processes_node);

    // explorer的pis标志永远是foreground，因为它必须相应如C+z这样的事件
    pcb->pis      = pis_foreground;
    pcb->disp_buf = alloc_con_buf();

    cur_fg_proc   = NULL;
    expl_proc     = pcb;
    expl_stat     = es_fg;

    // 初始化输入缓冲区
    expl_cmd_input_buf =
        (char*)alloc_static_kernel_mem(EXPL_CMD_INPUT_BUF_SIZE, 1);
    expl_cmd_input_buf[0]  = '\0';
    expl_cmd_input_size    = 0;
    
    cmd_char2(0, '_');

    scr_disp_caption("Display");
    scr_cmd_caption("Command");

    // 注册键盘消息
    register_key_msg();
    register_char_msg();

    set_intr_state(is);
}

/* 输出进程列表 */
static void show_procs()
{
    scr_disp_caption("Processes");
    scr_cmd_caption("[Q] Quit [N] Next Page [B] Last Page");
    


    struct sysmsg msg;
    while(peek_sysmsg(SYSMSG_SYSCALL_PEEK_OPERATION_REMOVE, &msg))
        ;
}

/* 提交并执行一条命令 */
static bool explorer_submit_cmd()
{
    bool ret = true;

    clr_cmd();
    cmd_char2(0, '_');

    if(strcmp(expl_cmd_input_buf, "shutdown") == 0)
        ret = false;
    else if(strcmp(expl_cmd_input_buf, "procs") == 0)
    {
        show_procs();
        scr_cmd_caption("Command");
    }

    expl_cmd_input_buf[0] = '\0';
    expl_cmd_input_size   = 0;

    return ret;
}

static void explorer_new_cmd_char(char ch)
{
    // 退格
    if(ch == '\b')
    {
        if(expl_cmd_input_size > 0)
        {
            cmd_char2(expl_cmd_input_size, ' ');
            expl_cmd_input_buf[--expl_cmd_input_size] = '\0';
            cmd_char2(expl_cmd_input_size, '_');
        }
        return;
    }

    // 命令缓冲区已满
    if(expl_cmd_input_size >= EXPL_CMD_INPUT_BUF_SIZE - 1)
        return;

    // 忽略换行符和制表符
    if(ch == '\n' || ch == '\t')
        return;

    cmd_char2(expl_cmd_input_size, ch);
    expl_cmd_input_buf[expl_cmd_input_size] = ch;
    expl_cmd_input_buf[++expl_cmd_input_size] = '\0';
    cmd_char2(expl_cmd_input_size, '_');
}

/* 将当前前台进程切换到后台 */
static void explorer_usr_interrupt()
{
    intr_state is = fetch_and_disable_intr();

    if(!cur_fg_proc)
    {
        ASSERT_S(expl_stat == es_fg);
        set_intr_state(is);
        return;
    }

    ASSERT_S(expl_stat == es_bg);

    cur_fg_proc->pis = pis_background;
    copy_scr_to_con_buf(cur_fg_proc);
    cur_fg_proc = NULL;
    copy_con_buf_to_scr(expl_proc);

    expl_stat = es_fg;

    set_intr_state(is);
}

/* 杀死当前前台进程 */
static void explorer_usr_exit()
{
    intr_state is = fetch_and_disable_intr();

    if(cur_fg_proc)
    {
        kill_process(cur_fg_proc);
        cur_fg_proc = NULL;
        copy_con_buf_to_scr(expl_proc);
        expl_stat = es_fg;
    }

    set_intr_state(is);
}

/*
    进行一次explorer状态转移
    返回false时停机

    IMPROVE：这段代码的逻辑表述实在丑……
*/
static bool explorer_transfer()
{
    if(expl_stat == es_fg)
    {
        struct sysmsg msg;
        if(peek_sysmsg(SYSMSG_SYSCALL_PEEK_OPERATION_REMOVE, &msg))
        {
            // 取得一条键盘按下的消息
            if(msg.type == SYSMSG_TYPE_KEYBOARD &&
               is_kbmsg_down(&msg))
            {
                uint8_t key = get_kbmsg_key(&msg);

                // 按回车提交一条命令
                if(key == VK_ENTER)
                    return explorer_submit_cmd();
            }
            else if(msg.type == SYSMSG_TYPE_CHAR) // 取得一条字符消息
            {
                explorer_new_cmd_char(get_chmsg_char(&msg));
                return true;
            }
        }
    }
    else if(expl_stat == es_bg)
    {
        struct sysmsg msg;
        if(peek_sysmsg(SYSMSG_SYSCALL_PEEK_OPERATION_REMOVE, &msg))
        {
            // 取得一条键盘按下的消息
            if(msg.type == SYSMSG_TYPE_KEYBOARD &&
               is_kbmsg_down(&msg))
            {
                uint8_t key = get_kbmsg_key(&msg);
                
                // 试图将当前前台程序切到后台
                if(key == 'Z' && is_key_pressed(VK_LCTRL))
                {
                    explorer_usr_interrupt();
                    return true;
                }

                // 试图干掉当前前台程序
                if(key == 'C' && is_key_pressed(VK_LCTRL))
                {
                    explorer_usr_exit();
                    return true;
                }
            }
        }
    }
    else // 暂时没有实现 explorer IO delegation，所以es_dg是非法状态
        FATAL_ERROR("invalid explorer state");

    return true;
}

void explorer()
{
    init_explorer();

    reformat_dp(0, DISK_PT_AFS);

    ipt_import_from_dp(get_dpt_unit(DPT_UNIT_COUNT - 1)->sector_begin);

    while(explorer_transfer())
        ;

    clr_scr();
    destroy_kernel();
    while(true)
        ;
}

void foreground_exit()
{
    // TODO
}
