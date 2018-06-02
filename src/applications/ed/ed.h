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
    int cur_x, cur_y;               // 在屏幕上的x、y坐标
    
    char *text;                     // 没错就是这么暴力（
    int text_buf_size;              // 缓冲区大小
    int gap_beg, gap_end;           // gap区间[beg, end)

    char *bg_buf;                   // 后台显示缓冲
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

/* editor主循环 */
void ed_mainloop(ed_t *ed);

#endif /* ED_ED_H */
