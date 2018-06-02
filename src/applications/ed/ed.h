#ifndef ED_ED_H
#define ED_ED_H

#include <shared/ptrlist.h>
#include <shared/sys.h>

/* editor状态 */
typedef struct
{
    filesys_dp_handle dp;           // 正在编辑的文件所处分区
    char *file;                     // 正在编辑的文件路径
    
    bool dirty;                     // 是否有未保存的内容

    int scr_top_lineno;             // 位于屏幕顶端位置是哪个显示行
    int cur_x, cur_y;               // 光标的全局位置
    
    char *text;                     // 没错就是这么暴力（
    size_t text_buf_size;           // 缓冲区大小
    size_t gap_beg, gap_end;           // gap区间[beg, end)
} ed_t;

/* 字符流接口 */
typedef char (*ED_TEXT_PROVIDER)();

/* 默认字符流：总是返回空字符 */
char default_ed_text_provider_null();

/* 从一个字符流创建一个editor */
ed_t *new_ed(filesys_dp_handle dp, const char *filename,
             ED_TEXT_PROVIDER text_provider);

/* 销毁一个编辑器 */
void free_ed(ed_t *ed);

/* 一般来说只需要第一次创建editor后调用，平时它会自己刷新屏幕 */
void render_ed(const ed_t *ed);

/* 进行一次状态转移，退出时返回false */
bool ed_trans(ed_t *ed);

#endif /* ED_ED_H */
