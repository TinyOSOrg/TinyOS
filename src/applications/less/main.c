#include <shared/screen.h>
#include <shared/string.h>
#include <shared/sys.h>

#include <lib/fstm.h>
#include <lib/input.h>
#include <lib/mem.h>
#include <lib/path.h>

char *fetch_from_file(filesys_dp_handle dp, const char *path)
{
    ifstm_t *f = ifstm_init(dp, path, FSTM_BUF_SIZE_DEFAULT);
    if(!f)
        return NULL;

    size_t size = ifstm_remain_size(f);
    char *ret = malloc(size + 1);
    if(!ret)
    {
        ifstm_free(f);
        return NULL;
    }

    char ch; size_t i = 0;
    while((ch = ifstm_next(f)) != FSTM_EOF)
        ret[i++] = ch;
    
    ifstm_free(f);
    ret[size] = '\0';
    return ret;
}

char *fetch_from_stdin()
{
    size_t buf_size = 16, idx = 0;
    char *buf = malloc(buf_size);
    if(!buf)
        return NULL;
    
    char ch;
    do {
        ch = get_char();
        if(idx >= buf_size)
        {
            char *new_buf = malloc(buf_size * 2);
            if(!new_buf)
            {
                free(buf);
                return NULL;
            }
            memcpy(new_buf, buf, buf_size);
            free(buf);
            buf = new_buf;
            buf_size *= 2;
        }
        buf[idx++] = ch;
    } while(ch);

    return buf;
}

char *fetch_input(const char *this_path, const char *dst_path)
{
    if(dst_path)
    {
        filesys_dp_handle dp;
        char *path = malloc_and_cat_path(this_path, dst_path, &dp);
        if(!path)
        {
            printf("%LInvalid filename: %s", dst_path);
            return NULL;
        }

        char *ret = fetch_from_file(dp, path);
        if(!ret)
        {
            printf("%LFailed to open file: %s", dst_path);
            return NULL;
        }

        return ret;
    }

    char *ret = fetch_from_stdin();
    if(!ret)
    {
        printf("%LMemory allocation failed...");
        return NULL;
    }
    return ret;
}

#define SCRW CON_BUF_ROW_SIZE
#define SCRH CON_BUF_COL_SIZE

#define CH_IDX(x, y) (2 * ((x) + (y) * SCRW))
#define AB_IDX(x, y) (CH_IDX((x), (y)) + 1)

void render_to(char *buf, int top_line, const char *text)
{
    int line = 0, ip = 0, x = 0, y = 0;
    char ch;

    // 清屏
    for(int i = 0; i < CON_BUF_CHAR_COUNT; ++i)
        buf[i * 2] = ' ';

    // 先找到第一个在屏幕内的字符
    while(line < top_line && (ch = text[ip++]))
    {
        if(ch == '\n' || ++x > SCRW)
        {
            ++line;
            x = 0;
        }
    }

    // 哦耶
    x = y = 0;
    while(y < SCRH && (ch = text[ip++]))
    {
        if(ch == '\n')
        {
            ++y;
            x = 0;
            continue;
        }

        if(ch == '\t')
            ch = ' ';

        buf[CH_IDX(x, y)] = ch;
        if(++x >= SCRW)
        {
            ++y;
            x = 0;
        }
    }
}

int try_move_down(int line_cnt, int top_line)
{
    if(top_line + SCRH < line_cnt)
        return top_line + 1;
    return top_line;
}

int try_move_up(int line_cnt, int top_line)
{
    if(top_line > 0)
        return top_line - 1;
    return 0;
}

int main(int argc, char *argv[])
{
    // 最多能查看一个文件或者从别的进程获取输入
    if(argc > 2)
    {
        printf("%LUsage: less filename");
        printf("%L       ... | less");
        return -1;
    }

    char *text = fetch_input(argv[0], argv[1]);
    if(!text)
        return -1;

    // 行数统计
    int line_cnt = 1, ip = 0, x = 0;
    char ch;
    while((ch = text[ip++]))
    {
        if(ch == '\n' || ++x >= SCRW)
        {
            ++line_cnt;
            x = 0;
        }
    }

    // 后台渲染缓冲
    char *bg_buf = malloc(CON_BUF_BYTE_SIZE);
    if(!bg_buf)
    {
        printf("%LFailed to allocate background rendering buffer");
        return -1;
    }

    // 渲染缓冲属性初始化
    for(int i = 0; i < CON_BUF_CHAR_COUNT; ++i)
        bg_buf[i * 2 + 1] = CH_GRAY | BG_BLACK;
    
    // 屏幕显示缓冲
    if(!alloc_con_buf())
    {
        printf("%LFailed to allocate screen buffer");
        return -1;
    }

    // 屏幕顶端行位置
    int top_line = 0;

    alloc_fg();
    register_key_msg();

    while(true)
    {
        render_to(bg_buf, top_line, text);
        set_scr(bg_buf);

        // 获取一个键盘按下的消息
        struct sysmsg msg;
        while(!peek_sysmsg(SYSMSG_SYSCALL_PEEK_OPERATION_REMOVE, &msg) ||
              msg.type != SYSMSG_TYPE_KEYBOARD || !is_kbmsg_down(&msg))
            wait_for_sysmsg();
        uint8_t key = get_kbmsg_key(&msg);

        switch(key)
        {
        case 'J':
        case VK_DOWN:
            top_line = try_move_down(line_cnt, top_line);
            break;
        case 'K':
        case VK_UP:
            top_line = try_move_up(line_cnt, top_line);
            break;
        case 'Q':
        case VK_ESCAPE:
            return 0;
        }
    }

    return 0;
}
