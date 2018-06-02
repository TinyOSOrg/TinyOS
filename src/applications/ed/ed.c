#include <shared/string.h>
#include <lib/mem.h>

#include "alloc.h"
#include "ed.h"

/* 初始文字缓冲区大小 */
#define INIT_TEXT_BUF_SIZE 128

/*
    将一块大小为old_size的缓存ori扩充为大小为new_size的缓存
    ori中的内容会被拷贝至新缓存的前old_size部分
*/
static void *expand_buffer(void *ori, size_t ori_size, size_t new_size)
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

    size_t text_len = 0;
    ed->text = malloc(INIT_TEXT_BUF_SIZE);
    ed->text_buf_size = INIT_TEXT_BUF_SIZE;

    char ch;
    while((ch = text_provider()))
    {
        // 检查缓冲区是否需要扩充
        if(text_len >= ed->text_buf_size)
        {
            ed->text = expand_buffer(ed->text,
                                     ed->text_buf_size,
                                     ed->text_buf_size * 2);
            ed->text_buf_size *= 2;
        }
        ed->text[text_len++] = ch;
    }

    ed->gap_beg = text_len;
    ed->gap_end = ed->text_buf_size;

    return ed;
}

void free_ed(ed_t *ed)
{
    if(ed->file)
        free(ed->file);
    free(ed->text);
    free(ed);
}

void render_ed(const ed_t *ed)
{
    printf("File size = %u\n", ed->text_buf_size - (ed->gap_end - ed->gap_beg));
}

bool ed_trans(ed_t *ed)
{
    if(is_key_pressed(VK_ESCAPE))
        return false;
    return true;
}
