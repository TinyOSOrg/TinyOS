#include <kernel/assert.h>
#include <kernel/console/console.h>
#include <kernel/explorer/cmds.h>
#include <kernel/explorer/disp.h>
#include <kernel/explorer/explorer.h>
#include <kernel/explorer/screen.h>
#include <kernel/filesys/dpt.h>
#include <kernel/filesys/import/import.h>
#include <kernel/interrupt.h>
#include <kernel/kernel.h>
#include <kernel/memory.h>
#include <kernel/process/process.h>
#include <kernel/execelf/execelf.h>

#include <shared/ctype.h>

#include <lib/string.h>
#include <lib/sys.h>

/*
    IMPROVE: 这个explorer是赶工的结果，写得烂如翔（
             谁来重写一个……
*/

/* explorer命令输入buffer大小，三行 */
#define CMD_INPUT_BUF_SIZE (SCR_CMD_WIDTH * SCR_CMD_HEIGHT)

/* explorer显示区和命令区的默认标题 */
#define DISP_TITLE ("Output")
#define CMD_TITLE ("Command")

/* explorer工作目录buffer大小 */
#define EXPL_WORKING_DIR_BUF_SIZE 256

/* explorer初始工作目录 */
#define INIT_WORKING_DIR ("/")

/* explorer应有的PID */
#define EXPL_PID 1

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

/* explorer命令输入buffer */
static char *expl_cmd_input_buf;
static uint32_t expl_cmd_input_size;

/* explorer工作路径buffer */
static filesys_dp_handle expl_working_dp;
static char *expl_working_dir;
static uint32_t expl_working_dir_len;

static void init_explorer()
{
    intr_state is = fetch_and_disable_intr();

    // explorer进程永远处于“前台”，以遍接收键盘消息
    // 话虽如此，并不能随便输出东西到系统控制台上

    struct PCB *pcb = get_cur_PCB();

    // explorer应该是1号进程
    ASSERT_S(pcb->pid == EXPL_PID);

    // 把自己从PCB列表中摘除
    erase_from_ilist(&pcb->processes_node);

    // explorer的pis标志永远是foreground，因为它必须相应如C+z这样的事件
    pcb->pis      = pis_foreground;

    cur_fg_proc   = NULL;
    expl_proc     = pcb;
    expl_stat     = es_fg;

    // 初始化输入缓冲区
    expl_cmd_input_buf =
        (char*)alloc_static_kernel_mem(CMD_INPUT_BUF_SIZE, 1);
    expl_cmd_input_buf[0]  = '\0';
    expl_cmd_input_size    = 0;

    // 初始化工作目录缓冲区
    expl_working_dp  = 0;
    expl_working_dir =
        (char*)alloc_static_kernel_mem(EXPL_WORKING_DIR_BUF_SIZE, 1);
    strcpy(expl_working_dir, INIT_WORKING_DIR);
    expl_working_dir_len = strlen(INIT_WORKING_DIR);

    cmd_char2(0, '_');

    scr_disp_caption(DISP_TITLE);
    scr_cmd_caption(CMD_TITLE);

    // 注册键盘消息
    register_key_msg();
    register_char_msg();

    init_disp();

    set_intr_state(is);
}

/* 把一个进程调到前台来，失败时（如进程不存在）返回false */
static bool make_proc_foreground(uint32_t pid)
{
    intr_state is = fetch_and_disable_intr();

    struct PCB *pcb = get_PCB_by_pid(pid);
    if(!pcb || pid < 2)
    {
        set_intr_state(is);
        return false;
    }

    ASSERT_S(expl_stat == es_fg && pcb->pis == pis_background);

    expl_stat      = es_bg;
    expl_proc->pis = pis_expl_foreground;
    pcb->pis       = pis_foreground;
    cur_fg_proc    = pcb;
    copy_scr_to_con_buf(expl_proc);
    copy_con_buf_to_scr(pcb);

    set_intr_state(is);

    return true;
}

/*
    匹配形如 str1 str2 ... strN 的命令，会改写buf为
        str1 \0 str2 \0 ... strN \0
    且每个str的首元素地址被分别放到strs中
    所有空白字符均会被跳过
    返回识别到的str数量，超出max_strs_cnt的str被忽略
*/
static uint32_t expl_parse_cmd(char *buf, const char **strs,
                               uint32_t max_strs_cnt)
{
    uint32_t ri = 0, wi = 0; //ri用于读buf，wi用于改写buf
    uint32_t cnt = 0;

    while(true)
    {
        // 跳过空白字符
        while(buf[ri] && isspace(buf[ri]))
            ++ri;

        // 到头了，该结束了
        if(!buf[ri])
            break;

        // 取得一条str
        uint32_t str_beg = wi;
        while(buf[ri] && !isspace(buf[ri]))
            buf[wi++] = buf[ri++];
        strs[cnt] = &buf[str_beg];
        ++cnt;

        // 把str末端置为\0
        if(buf[ri])
            ++ri;
        buf[wi++] = '\0';
        
        if(cnt >= max_strs_cnt)
            break;
    }

    return cnt;
}

/* 就是一条一条地暴力匹配命令，反正这个不是很重要，先用着吧…… */
static bool explorer_exec_cmd(const char *strs[], uint32_t str_cnt)
{
    const char *cmd    = strs[0];
    const char **args  = &strs[1];
    uint32_t arg_cnt   = str_cnt - 1;

    if(strcmp(cmd, "cd") == 0)
    {
        if(arg_cnt != 1)
            goto INVALID_ARGUMENT;
        
        disp_new_line();
        expl_cd(&expl_working_dp, expl_working_dir, &expl_working_dir_len,
                EXPL_WORKING_DIR_BUF_SIZE - 1, args[0]);
    }
    else if(strcmp(cmd, "clear") == 0)
    {
        if(arg_cnt)
            goto INVALID_ARGUMENT;

        clr_disp();
        disp_set_cursor(0, 0);
    }
    else if(strcmp(cmd, "dp") == 0)
    {
        if(arg_cnt)
            goto INVALID_ARGUMENT;
        
        disp_new_line();
        expl_dp(expl_working_dp);
    }
    else if(strcmp(cmd, "exec") == 0)
    {
        if(!arg_cnt)
            goto INVALID_ARGUMENT;
        
        disp_new_line();
        expl_exec(expl_working_dp, expl_working_dir, args, arg_cnt);
    }
    else if(strcmp(cmd, "exit") == 0)
    {
        if(arg_cnt)
            goto INVALID_ARGUMENT;

        return false;
    }
    else if(strcmp(cmd, "fg") == 0)
    {
        uint32_t pid;
        if(arg_cnt != 1 || !str_to_uint32(args[0], &pid))
            goto INVALID_ARGUMENT;

        if(!make_proc_foreground(pid))
        {
            disp_new_line();
            disp_printf("Invalid pid");
        }
    }
    else if(strcmp(cmd, "ls") == 0)
    {
        if(arg_cnt)
            goto INVALID_ARGUMENT;
        
        disp_new_line();
        expl_ls(expl_working_dp, expl_working_dir);
    }
    else if(strcmp(cmd, "mkdir") == 0)
    {
        if(arg_cnt != 1)
            goto INVALID_ARGUMENT;
        
        disp_new_line();
        expl_mkdir(expl_working_dp, expl_working_dir, args[0]);
    }
    else if(strcmp(cmd, "ps") == 0)
    {
        if(arg_cnt)
            goto INVALID_ARGUMENT;

        disp_new_line();
        expl_show_procs();
    }
    else if(strcmp(cmd, "pwd") == 0)
    {
        if(arg_cnt)
            goto INVALID_ARGUMENT;

        disp_new_line();
        disp_printf("Current working directory: %u:%s",
                    expl_working_dp, expl_working_dir);
    }
    else if(strcmp(cmd, "rmdir") == 0)
    {
        if(arg_cnt != 1)
            goto INVALID_ARGUMENT;
        
        disp_new_line();
        expl_rmdir(expl_working_dp, expl_working_dir, args[0]);
    }
    else
    {
        disp_new_line();
        disp_printf("Invalid command");
    }

    return true;

INVALID_ARGUMENT:

    disp_new_line();
    disp_printf("Invalid argument(s)");
    return true;
}

/* 提交并执行一条命令 */
static bool explorer_submit_cmd()
{
    bool ret = true;

    clr_cmd();

    // 命令参数解析
    const char *strs[EXEC_ELF_ARG_MAX_COUNT];
    uint32_t str_cnt = expl_parse_cmd(
        expl_cmd_input_buf, strs, EXEC_ELF_ARG_MAX_COUNT);
    
    if(str_cnt)
        ret = explorer_exec_cmd(strs, str_cnt);

    expl_cmd_input_buf[0] = '\0';
    expl_cmd_input_size   = 0;

    scr_disp_caption(DISP_TITLE);
    scr_cmd_caption(CMD_TITLE);

    cmd_char2(0, '_');

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
    if(expl_cmd_input_size >= CMD_INPUT_BUF_SIZE - 1)
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
    expl_proc->pis = pis_foreground;

    set_intr_state(is);
}

/* 杀死当前前台进程 */
static void explorer_usr_exit()
{
    intr_state is = fetch_and_disable_intr();

    if(cur_fg_proc)
    {
        // 预先把目标进程的pis设置成后台
        // 这样杀死它就不会通知explorer导致重复copy显示数据什么的
        cur_fg_proc->pis = pis_background;

        kill_process(cur_fg_proc);
        cur_fg_proc    = NULL;
        expl_stat      = es_fg;
        expl_proc->pis = pis_foreground;
        copy_con_buf_to_scr(expl_proc);
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
                    clr_sysmsgs();
                    return true;
                }

                // 试图干掉当前前台程序
                if(key == 'C' && is_key_pressed(VK_LCTRL))
                {
                    explorer_usr_exit();
                    clr_sysmsgs();
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

    for(int i = 0; i < 5; ++i)
    {
        char name[30] = "t"; char idx_buf[20];
        uint32_to_str(i, idx_buf);
        strcat(name, idx_buf);
        exec_elf(name, 0, "/hello_world.elf", false, 0, NULL);
    }

    while(explorer_transfer())
        ;

    destroy_kernel();
    clr_scr();
    while(true)
        ;
}

void foreground_exit(struct PCB *pcb)
{
    intr_state is = fetch_and_disable_intr();
    
    if(pcb != cur_fg_proc)
    {
        set_intr_state(is);
        return;
    }

    cur_fg_proc    = NULL;
    expl_stat      = es_fg;
    expl_proc->pis = pis_foreground;
    copy_con_buf_to_scr(expl_proc);

    set_intr_state(is);
}
