#include <kernel/assert.h>
#include <kernel/explorer/procs.h>
#include <kernel/explorer/screen.h>
#include <kernel/interrupt.h>
#include <kernel/process/process.h>

#include <shared/bool.h>
#include <shared/string.h>
#include <shared/utility.h>

#include <lib/sys.h>

struct proc_info
{
    uint32_t pid;
    char name[PROCESS_NAME_MAX_LENGTH + 1];
};

STATIC_ASSERT(sizeof(struct proc_info) * MAX_PROCESS_COUNT <= 4096,
              invalid_size_of_proc_info_buffer);

/* 将所有进程的显示数据放入buf数组中 */
static void get_procs_info(struct proc_info *buf, uint32_t *cnt)
{
    ASSERT_S(buf && cnt);
    intr_state is = fetch_and_disable_intr();

    uint32_t _cnt = 0;

    // 跳过0号bootloader进程和1号explorer进程，其他进程都计入表中
    for(uint32_t pid = 2; pid < MAX_PROCESS_COUNT; ++pid)
    {
        struct PCB *pcb = get_PCB_by_pid(pid);
        if(!pcb)
            continue;
        buf[_cnt].pid = pcb->pid;
        strcpy_s(buf[_cnt].name, pcb->name,
                 PROCESS_NAME_MAX_LENGTH + 1);
        ++_cnt;
    }

    set_intr_state(is);

    *cnt = _cnt;
}

/* 每个显示页能放多少条进程信息 */
#define PROCS_PER_PAGE SCR_DISP_HEIGHT

/* 在显示区输出一页进程信息 */
static void show_procs_page(struct proc_info *buf,
                            uint32_t cnt, uint32_t page_idx)
{
    clr_disp();

    uint32_t y = 0, idx = page_idx * PROCS_PER_PAGE;
    while(y < SCR_DISP_HEIGHT && idx < cnt)
    {
        struct proc_info *info = &buf[idx];

        disp_char(0, y, ' ');
        uint32_t x = 1;

        char pid_str[20]; uint32_t si = 0;
        uint32_to_str(info->pid, pid_str);
        while(x < SCR_DISP_WIDTH && pid_str[si])
        {
            disp_char(x, y, pid_str[si]);
            ++x, ++si;
        }

        while(si < 6)
        {
            disp_char(x, y, ' ');
            ++x, ++si;
        }

        si = 0;
        while(x < SCR_DISP_WIDTH && info->name[si])
        {
            disp_char(x, y, info->name[si]);
            ++x, ++si;
        }

        ++y; ++idx;
    }
}

void expl_show_procs()
{
    scr_disp_caption("Processe List");
    scr_cmd_caption("[Q] Quit [N] Next Page [B] Last Page");

    clr_disp(); clr_cmd();

    // 取得所有进程信息
    struct proc_info *buf = (struct proc_info*)alloc_ker_page(false);
    if(!buf)
        return;
    uint32_t cnt;
    get_procs_info(buf, &cnt);

    uint32_t max_page_idx = ceil_int_div(cnt, PROCS_PER_PAGE) - 1;
    uint32_t page_idx     = 0;

    show_procs_page(buf, cnt, page_idx);

    while(true)
    {
        // 取得一条键盘按下的消息
        struct sysmsg msg;
        if(!peek_sysmsg(SYSMSG_SYSCALL_PEEK_OPERATION_REMOVE, &msg))
            continue;
        if(msg.type != SYSMSG_TYPE_KEYBOARD)
            continue;
        uint8_t key = get_kbmsg_key(&msg);

        // Quit
        if(key == 'Q')
            break;
        
        // 下一页
        if(key == 'N' && page_idx < max_page_idx)
        {
            ++page_idx;
            show_procs_page(buf, cnt, page_idx);
        }

        // 上一页
        if(key == 'B' && page_idx > 0)
        {
            --page_idx;
            show_procs_page(buf, cnt, page_idx);
        }
    }

    free_ker_page(buf);
    clr_sysmsgs();
}
