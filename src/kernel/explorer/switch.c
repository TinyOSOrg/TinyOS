#include <kernel/assert.h>
#include <kernel/console/console.h>
#include <kernel/explorer/switch.h>
#include <kernel/interrupt.h>

#include <shared/string.h>

void switch_to_background(struct PCB *pcb)
{
    intr_state is = fetch_and_disable_intr();

    struct con_buf *disp = pcb->disp_buf;

    // 这里只需要锁系统控制台而不需要锁进程显示缓冲区，是因为前台进程不可能正在使用自己的后台显示缓冲
    if(disp)
        semaphore_wait(&get_sys_con_buf()->lock);

    pcb->pis = pis_background;
    if(disp)
    {
        memcpy(disp->data, get_sys_con_buf()->data, CON_BUF_BYTE_SIZE);
        disp->cursor = get_sys_con_buf()->cursor;
    }

    if(disp)
        semaphore_signal(&get_sys_con_buf()->lock);

    set_intr_state(is);
}

void switch_to_foreground(struct PCB *pcb)
{
    intr_state is = fetch_and_disable_intr();

    struct con_buf *disp = pcb->disp_buf;

    // 需要同时锁系统控制台和进程显示缓冲区

    if(disp)
    {
        semaphore_wait(&get_sys_con_buf()->lock);
        semaphore_wait(&disp->lock);
    }

    pcb->pis = pis_foreground;
    if(disp)
    {
        memcpy(get_sys_con_buf()->data, disp->data, CON_BUF_BYTE_SIZE);
        get_sys_con_buf()->cursor = disp->cursor;
    }

    if(disp)
    {
        semaphore_signal(&disp->lock);
        semaphore_signal(&get_sys_con_buf()->lock);
    }

    set_intr_state(is);
}
