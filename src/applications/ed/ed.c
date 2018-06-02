#include <shared/ctype.h>
#include <shared/screen.h>
#include <shared/string.h>
#include <shared/utility.h>

#include <lib/assert.h>
#include <lib/mem.h>

#include "ed.h"

/* 初始文字缓冲区大小 */
#define INIT_TEXT_BUF_SIZE 128

#define SCR_WIDTH  (CON_BUF_ROW_SIZE - 0)
#define SCR_HEIGHT (CON_BUF_COL_SIZE - 1)

/*
    将一块大小为old_size的缓存ori扩充为大小为new_size的缓存
    ori中的内容会被拷贝至新缓存的前old_size部分
*/
static void *expand_buffer_back(void *ori, size_t ori_size, size_t new_size)
{
    void *ret = malloc(new_size);
    memcpy(ret, ori, ori_size);
    free(ori);
    return ret;
}

char default_ed_text_provider_null()
{
    return '\0';
}

ed_t *new_ed(filesys_dp_handle dp, const char *filename,
             ED_TEXT_PROVIDER text_provider)
{
    ed_t *ed = malloc(sizeof(ed_t));
    if(!ed)
        return NULL;
    
    ed->dp = dp;
    if(filename)
    {
        ed->file = malloc(strlen(filename) + 1);
        strcpy(ed->file, filename);
    }
    else
        ed->file = NULL;

    ed->dirty = false;

    ed->scr_top_lineno = 0;
    ed->cur_x = ed->cur_y = 0;
    ed->cur_exp_x = 0;

    int text_len = 0;
    ed->text = malloc(INIT_TEXT_BUF_SIZE);
    ed->text_buf_size = INIT_TEXT_BUF_SIZE;

    char ch;
    while((ch = text_provider()))
    {
        // 检查缓冲区是否需要扩充
        if(text_len >= ed->text_buf_size)
        {
            ed->text = expand_buffer_back(ed->text,
                                          ed->text_buf_size,
                                          ed->text_buf_size * 2);
            ed->text_buf_size *= 2;
        }
        ed->text[text_len++] = ch;
    }

    for(int i = text_len, j = ed->text_buf_size; i > 0; --i, --j)
        ed->text[j - 1] = ed->text[i - 1];

    ed->gap_beg = 0;
    ed->gap_end = ed->text_buf_size - text_len;

    ed->bg_buf = malloc(CON_BUF_BYTE_SIZE);

    return ed;
}

void free_ed(ed_t *ed)
{
    if(ed->file)
        free(ed->file);
    free(ed->text);
    free(ed->bg_buf);
    free(ed);
}

#define NOR_TXT_ATTRIB (CH_GRAY  | BG_BLACK)
#define CUR_TXT_ATTRIB (CH_BLACK | BG_GRAY)

/* 在text中找到第一个在屏幕范围内的字符 */
static int find_fst_ch_in_scr(const ed_t *ed)
{
    int ip = 0, lineno = 0, x = 0;
    while(lineno < ed->scr_top_lineno)
    {
        if(ip == ed->gap_beg)
            ip = ed->gap_end;
        if(ip >= ed->text_buf_size)
            break;

        char ch = ed->text[ip++];
        if(ch == '\n' || ++x >= SCR_WIDTH)
        {
            ++lineno;
            x = 0;
        }
    }
    return ip;
}

/* 绘制状态栏 */
static void draw_state_bar(const ed_t *ed)
{
    char buf[CON_BUF_ROW_SIZE];
    memset(buf, ' ', sizeof(buf));

    char *p = buf;
    strcpy(p, ed->dirty ? "Unsaved = [*] Line = " :
                          "Unsaved = [ ] Line = ");
    
    p += strlen(p); uint32_to_str(1 + ed->scr_top_lineno + ed->cur_y, p);
    p += strlen(p); strcpy(p, " Col = ");
    p += strlen(p); uint32_to_str(1 + ed->cur_x, p);
    p += strlen(p); strcpy(p, " File = ");
    p += strlen(p); uint32_to_str(ed->dp, p);
    p += strlen(p); strcpy(p, ":");
    p += strlen(p); strcpy_s(p, ed->file, CON_BUF_BYTE_SIZE - (p - buf));

    char *dst = ed->bg_buf + SCR_HEIGHT * SCR_WIDTH * 2;
    for(int x = 0; x < CON_BUF_ROW_SIZE; ++x)
    {
        dst[x * 2]     = buf[x];
        dst[x * 2 + 1] = buf[x] == '*' ? CH_RED | CH_LIGHT | BG_BLACK :
                                         NOR_TXT_ATTRIB;
    }
}

#define CHAR_IDX(x, y) (2 * ((x) + (y) * CON_BUF_ROW_SIZE))
#define ATTRIB_IDX(x, y) (CHAR_IDX((x), (y)) + 1)

/* 重绘编辑区，双缓冲避免闪烁 */
static void refresh_ed(ed_t *ed)
{
    for(int i = 0; i < CON_BUF_CHAR_COUNT; ++i)
    {
        ed->bg_buf[i * 2]     = ' ';
        ed->bg_buf[i * 2 + 1] = NOR_TXT_ATTRIB;
    }

    int ip = find_fst_ch_in_scr(ed);

    int x = 0, y = 0;
    while(y < SCR_HEIGHT)
    {
        if(ip == ed->gap_beg)
        {
            ip = ed->gap_end;
            ed->bg_buf[ATTRIB_IDX(x, y)] = CUR_TXT_ATTRIB;
            ed->cur_x = x, ed->cur_y = y;
        }

        if(ip >= ed->text_buf_size)
            break;
        
        char ch = ed->text[ip++];
        if(ch == '\n')
        {
            x = 0;
            ++y;
        }
        else
        {
            if(ch == '\t')
                ch = ' ';
            ed->bg_buf[CHAR_IDX(x, y)] = ch;
            if(++x >= SCR_WIDTH)
            {
                x = 0;
                ++y;
            }
        }
    }

    draw_state_bar(ed);
    set_scr(ed->bg_buf);
}

/*
    移动ed中gap的位置
    IMPROVE：这里建了个临时缓冲来搬运数据，显然可以优化
*/
static void mov_gap(ed_t *ed, int new_beg)
{
    int gap_size = ed->gap_end - ed->gap_beg;
    int new_end = new_beg + gap_size;

    // 需要重新安排[ra_beg, ra_end)间的所有数据
    int ra_beg = MIN(new_beg, ed->gap_beg);
    int ra_end = MAX(new_end, ed->gap_end);
    int ra_size = ra_end - ra_beg;

    int md_size = ra_size - gap_size;
    char *buf = malloc(md_size);

    for(int i = ra_beg, j = 0; j < md_size; ++i, ++j)
    {
        if(i == ed->gap_beg)
            i = ed->gap_end;
        buf[j] = ed->text[i];
    }

    for(int i = ra_beg, j = 0; j < md_size; ++i, ++j)
    {
        if(i == new_beg)
            i = new_end;
        ed->text[i] = buf[j];
    }

    free(buf);

    ed->gap_beg = new_beg;
    ed->gap_end = new_end;
}

/* 光标上移 */
static void cur_up(ed_t *ed)
{
    int new_beg = MAX(0, ed->gap_beg - 1);
    
    // 先搜索到上一行的开头
    bool new_line = false;
    while(new_beg > 0)
    {
        if(ed->text[new_beg] == '\n')
        {
            if(new_line)
            {
                ++new_beg;
                break;
            }
            new_line = true;
        }
        --new_beg;
    }

    if(!new_line)
    {
        ed->cur_exp_x = 0;
        new_beg = 0; // 纳尼，这就是第一行
    }
    else
    {
        int new_x = 0;
        while(new_x < ed->cur_exp_x && ed->text[new_beg + new_x] != '\n')
            ++new_x;
        new_beg += new_x;
    }

    // 是否要向上卷一屏幕
    if(new_line && --ed->cur_y < 0)
        --ed->scr_top_lineno;
    ed->scr_top_lineno = MAX(0, ed->scr_top_lineno);
    
    mov_gap(ed, new_beg);
    refresh_ed(ed);
}

/* 光标下移 */
static void cur_down(ed_t *ed)
{
    int new_beg = ed->gap_end;

    int new_x = ed->cur_x;
    bool new_line = false;
    while(new_beg < ed->text_buf_size - 1)
    {
        if(ed->text[new_beg] == '\n')
        {
            if(new_line)
                break;
            new_line = true;
            new_x    = 0;
        }
        else if(new_line)
        {
            if(new_x >= ed->cur_exp_x)
                break;
            if(++new_x >= ed->cur_exp_x)
            {
                ++new_beg;
                break;
            }
        }
        else
        {
            if(++new_x >= SCR_WIDTH)
            {
                new_x = 0;
                new_line = true;
            }
        }
        ++new_beg;
    }

    if(new_beg >= ed->text_buf_size - 1)
        ed->cur_exp_x = new_x;

    new_beg -= (ed->gap_end - ed->gap_beg);

    // 是否要往下卷一行屏幕
    if(new_line && ++ed->cur_y >= SCR_HEIGHT)
        ++ed->scr_top_lineno;

    mov_gap(ed, new_beg);
    refresh_ed(ed);
}

/* 光标左移 */
static void cur_left(ed_t *ed)
{
    ed->cur_exp_x = ed->cur_x;
    if(ed->gap_beg <= 0)
        return;
    int new_beg = ed->gap_beg - 1;
    if(ed->text[new_beg] == '\n')
        return;
    mov_gap(ed, new_beg);
    refresh_ed(ed);
    ed->cur_exp_x = ed->cur_x;
}

/* 光标右移 */
static void cur_right(ed_t *ed)
{
    ed->cur_exp_x = ed->cur_x;
    if(ed->gap_end >= ed->text_buf_size)
        return;
    if(ed->text[ed->gap_end] == '\n')
        return;
    mov_gap(ed, ed->gap_beg + 1);
    refresh_ed(ed);
    ed->cur_exp_x = ed->cur_x;
}

#define GAP_EXPAND_UNIT 256

/* 扩充gap大小 */
static void expand_gap_buffer(ed_t *ed)
{
    int new_buf_size = ed->text_buf_size + GAP_EXPAND_UNIT;
    int new_gap_beg  = ed->gap_beg;
    int new_gap_end  = ed->gap_end + GAP_EXPAND_UNIT;

    char *new_buf = malloc(new_buf_size);

    int ch_cnt = ed->text_buf_size - (ed->gap_end - ed->gap_beg);
    for(int i = 0, c = 0, j = 0; c < ch_cnt; ++i, ++c, ++j)
    {
        if(i == new_gap_beg)
            i = new_gap_end;
        if(j == ed->gap_beg)
            j = ed->gap_end;
        new_buf[i] = ed->text[j];
    }

    free(ed->text);
    ed->gap_beg = new_gap_beg;
    ed->gap_end = new_gap_end;
    ed->text_buf_size = new_buf_size;
    ed->text = new_buf;
}

/* 退格 */
static void bs_ed(ed_t *ed)
{
    if(ed->gap_beg <= 0)
        return;
    bool new_line = false;
    if(ed->text[--ed->gap_beg] == '\n' || ed->cur_x == 0)
        new_line = true;
    
    if(new_line && --ed->cur_y <= 0)
        --ed->scr_top_lineno;
    ed->scr_top_lineno = MAX(0, ed->scr_top_lineno);
    
    refresh_ed(ed);
    ed->cur_exp_x = ed->cur_x;
}

/* 输入一个字符 */
static void enter_char(ed_t *ed, char ch)
{
    ed->dirty = true;

    if(ch == '\b')
    {
        bs_ed(ed);
        return;
    }

    // 检查是否需要扩充buffer
    if(ed->gap_end <= ed->gap_beg)
        expand_gap_buffer(ed);
    
    ed->text[ed->gap_beg++] = ch;

    bool new_line = false;
    if(ch == '\n' || ++ed->cur_x >= SCR_WIDTH)
        new_line = true;
    
    if(new_line && ++ed->cur_y >= SCR_HEIGHT)
        ++ed->scr_top_lineno;
    
    refresh_ed(ed);
    ed->cur_exp_x = ed->cur_x;
}

/* 保存文件 */
static void save_file(ed_t *ed)
{
    if(!ed->file || !ed->dirty)
        return;
    if(remove_file(ed->dp, ed->file) != filesys_opr_success ||
       make_file(ed->dp, ed->file) != filesys_opr_success)
        return;
    
    usr_file_handle fp;
    if(open_file(ed->dp, ed->file, true, &fp) != filesys_opr_success)
        return;
    
    uint32_t fpos = 0;
    if(ed->gap_beg)
    {
        write_file(fp, fpos, ed->gap_beg, ed->text);
        fpos += ed->gap_beg;
    }

    if(ed->gap_end < ed->text_buf_size)
    {
        write_file(fp, fpos, ed->text_buf_size - ed->gap_end,
                             ed->text + ed->gap_end);
    }

    close_file(fp);
    ed->dirty = false;
}

/* 进行一次状态转移 */
bool ed_trans(ed_t *ed)
{
    struct sysmsg msg;
    while(!peek_sysmsg(SYSMSG_SYSCALL_PEEK_OPERATION_REMOVE, &msg))
        wait_for_sysmsg();

    if(msg.type == SYSMSG_TYPE_CHAR)
    {
        bool ctrl = is_key_pressed(VK_LCTRL);
        char ch = get_chmsg_char(&msg);

        if(ctrl)
        {
            switch(to_lower(ch))
            {
            case 'q':
                if(!ed->dirty || is_key_pressed(VK_LSHIFT))
                    return false;
                break;
            case 'j': cur_left(ed);  break;
            case 'l': cur_right(ed); break;
            case 'i': cur_up(ed);    break;
            case 'k': cur_down(ed);  break;
            case 's': save_file(ed); break;
            }
        }
        else
            enter_char(ed, ch);
    }
    else if(msg.type == SYSMSG_TYPE_KEYBOARD && is_kbmsg_down(&msg))
    {
        uint8_t key = get_kbmsg_key(&msg);
        switch(key)
        {
        case VK_LEFT:  cur_left(ed);  break;
        case VK_RIGHT: cur_right(ed); break;
        case VK_UP:    cur_up(ed);    break;
        case VK_DOWN:  cur_down(ed);  break;
        }
    }
    
    return true;
}

void ed_mainloop(ed_t *ed)
{
    refresh_ed(ed);
    while(ed_trans(ed))
        ;
}
