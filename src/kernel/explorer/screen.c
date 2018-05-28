#include <kernel/assert.h>
#include <kernel/console/console.h>
#include <kernel/explorer/screen.h>
#include <kernel/interrupt.h>
#include <kernel/process/process.h>

#include <shared/screen.h>
#include <shared/string.h>

#include <lib/sys.h>

#define CMD_X_BASE 0
#define CMD_Y_BASE (1 + SCR_DISP_HEIGHT + 1)

#define DISP_X_BASE 0
#define DISP_Y_BASE 1

#define BG_ATTRIB(X, Y) \
    do { \
        set_char_attrib_row_col((Y), (X), \
                                CH_BLACK | BG_GRAY); \
    } while(0)

#define BG(X, Y) \
    do { \
        set_char_row_col((Y), (X), ' '); \
        BG_ATTRIB((X), (Y)); \
    } while(0)

#define CLR_ATTRIB(X, Y) \
    do { \
        set_char_attrib_row_col((Y), (X), \
                                CH_GRAY | BG_BLACK); \
    } while(0)

#define CLR(X, Y) \
    do { \
        set_char_row_col((Y), (X), ' '); \
        CLR_ATTRIB((X), (Y)); \
    } while(0)

static void caption(uint32_t x_offset, uint32_t y, const char *title)
{
    for(uint32_t x = 0; x < x_offset; ++x)
        BG(x, y);
    uint32_t x = x_offset, idx = 0;

    while(title[idx] && x < CON_BUF_ROW_SIZE)
    {
        BG_ATTRIB(x, y);
        set_char_row_col(y, x, title[idx]);
        ++x, ++idx;
    }

    while(x < CON_BUF_ROW_SIZE)
    {
        BG(x, y);
        ++x;
    }
}

static void clr_rect(uint32_t xbeg, uint32_t xend,
                          uint32_t ybeg, uint32_t yend)
{
    for(uint32_t x = xbeg; x < xend; ++x)
    {
        for(uint32_t y = ybeg; y < yend; ++y)
            CLR(x, y);
    }
}

void clr_scr()
{
    clr_rect(0, CON_BUF_ROW_SIZE, 0, CON_BUF_COL_SIZE);
}

void scr_disp_caption(const char *title)
{
    caption(1, 0, title);
}

void scr_cmd_caption(const char *title)
{
    caption(1, 1 + SCR_DISP_HEIGHT, title);
}

void clr_disp()
{
    clr_rect(DISP_X_BASE, DISP_X_BASE + SCR_DISP_WIDTH,
             DISP_Y_BASE, DISP_Y_BASE + SCR_DISP_HEIGHT);
}

void clr_cmd()
{
    clr_rect(CMD_X_BASE, CMD_X_BASE + SCR_CMD_WIDTH,
             CMD_Y_BASE, CMD_Y_BASE + SCR_CMD_HEIGHT);
}

void disp_char(uint8_t x, uint8_t y, char ch)
{
    set_char_row_col(y + DISP_Y_BASE, x + DISP_X_BASE, ch);
}

void disp_char2(uint16_t pos, char ch)
{
    set_char_row_col(pos / SCR_DISP_WIDTH + DISP_Y_BASE,
                     pos % SCR_DISP_WIDTH + DISP_X_BASE,
                     ch);
}

void disp_attrib(uint8_t x, uint8_t y, uint8_t attrib)
{
    set_char_attrib_row_col(y + DISP_Y_BASE, x + DISP_X_BASE, attrib);
}

void disp_attrib2(uint16_t pos, uint8_t attrib)
{
    set_char_attrib_row_col(pos / SCR_DISP_WIDTH + DISP_Y_BASE,
                            pos % SCR_DISP_WIDTH + DISP_X_BASE,
                            attrib);
}

void disp_roll_screen()
{
    roll_scr(DISP_Y_BASE, DISP_Y_BASE + SCR_DISP_HEIGHT);
}

void cmd_char(uint8_t x, uint8_t y, char ch)
{
    set_char_row_col(y + CMD_Y_BASE, x + CMD_X_BASE, ch);
}

void cmd_char2(uint16_t pos, char ch)
{
    set_char_row_col(pos / SCR_CMD_WIDTH + CMD_Y_BASE,
                     pos % SCR_CMD_WIDTH + CMD_X_BASE,
                     ch);
}

void cmd_attrib(uint8_t x, uint8_t y, uint8_t attrib)
{
    set_char_attrib_row_col(y + CMD_Y_BASE, x + CMD_X_BASE, attrib);
}

void cmd_attrib2(uint16_t pos, uint8_t attrib)
{
    set_char_attrib_row_col(pos / SCR_CMD_WIDTH + CMD_Y_BASE,
                            pos % SCR_CMD_WIDTH + CMD_X_BASE,
                            attrib);
}

void disp_show_str(uint8_t x, uint8_t y, const char *str)
{
    uint32_t idx = 0;
    while(x < SCR_CMD_WIDTH && str[idx])
        disp_char(x++, y, str[idx++]);
}

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
