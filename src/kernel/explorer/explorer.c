#include <kernel/assert.h>
#include <kernel/console/console.h>
#include <kernel/explorer/explorer.h>
#include <kernel/filesys/dpt.h>
#include <kernel/filesys/import/import.h>
#include <kernel/interrupt.h>
#include <kernel/kernel.h>
#include <kernel/memory.h>
#include <kernel/process/process.h>
#include <kernel/execelf/execelf.h>

#include <shared/string.h>

#include <lib/conio.h>
#include <lib/filesys.h>
#include <lib/keyboard.h>
#include <lib/proc.h>

/*
    Explorer是个FSA，消息驱动（键盘消息和其他进程的特殊调用），转换如下：
        // TODO
*/

/* Explorer状态 */
enum explorer_state
{
    es_fg,  // explorer本身处于前台，可接受输入
    es_dg,  // 有前台进程把输入输出托管到explorer
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

/* explorer命令输入缓冲大小 */
#define EXPL_CMD_INPUT_BUF_SIZE 4096

/* explorer行输入缓冲大小 */
#define EXPL_LINE_INPUT_BUF_SIZE 64

/* explorer命令输入缓冲 */
static char *expl_cmd_input_buf;
static uint32_t expl_cmd_input_size;

/* explorer行输入缓冲 */
static char expl_line_input_buf[EXPL_LINE_INPUT_BUF_SIZE];
static uint32_t expl_line_input_size;

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

void copy_con_buf_to_scr(struct PCB *pcb)
{
    intr_state is = fetch_and_disable_intr();

    struct con_buf *disp = pcb->disp_buf;

    // 需要同时锁系统控制台和进程显示缓冲区

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

    // 把自己从PCB列表中摘除
    erase_from_ilist(&pcb->processes_node);

    // explorer的pis标志永远是foreground，因为它必须相应如C+z这样的事件
    pcb->pis      = pis_foreground;
    pcb->disp_buf = alloc_con_buf();

    cur_fg_proc   = NULL;
    expl_proc     = pcb;
    expl_stat     = es_fg;

    // 初始化输入缓冲区
    expl_cmd_input_buf     = (char*)alloc_ker_page(false);
    expl_cmd_input_buf[0]  = '\0';
    expl_cmd_input_size    = 0;
    expl_line_input_size   = 0;
    expl_line_input_buf[0] = '\0';

    set_intr_state(is);
}

/*
    进行一次explorer状态转移
    返回false时停机
*/
static bool explorer_transfer()
{
    if(is_key_pressed(VK_ESCAPE))
        return false;

    if(expl_stat == es_fg)
    {

    }
    else if(expl_stat == es_bg)
    {

    }
    else // 暂时没有实现 explorer IO delegation，所以es_dg是非法状态
        FATAL_ERROR("invalid explorer state");

    return true;
}

void explorer()
{
    init_explorer();

    printf("Explorer process, pid = %u\n", get_pid());

    reformat_dp(0, DISK_PT_AFS);

    ipt_import_from_dp(get_dpt_unit(DPT_UNIT_COUNT - 1)->sector_begin);

    exec_elf("test elf", 0, "/minecraft.txt", false, 0, NULL);

    while(explorer_transfer())
        ;
    
    destroy_kernel();
    while(true)
        ;
}

void foreground_exit()
{
    // TODO
}
