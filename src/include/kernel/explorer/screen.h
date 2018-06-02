#ifndef TINY_OS_EXPLORER_SCREEN_H
#define TINY_OS_EXPLORER_SCREEN_H

#include <shared/screen.h>
#include <shared/stdint.h>

struct PCB;

#define SCR_CMD_WIDTH  (CON_BUF_ROW_SIZE - 0)
#define SCR_CMD_HEIGHT 3

#define SCR_DISP_WIDTH  SCR_CMD_WIDTH
#define SCR_DISP_HEIGHT (CON_BUF_COL_SIZE - 2 - SCR_CMD_HEIGHT)

/* 清除整个屏幕的内容 */
void scr_clr();

/* 绘制展示区标题栏 */
void scr_disp_caption(const char *title);

/* 绘制命令区标题栏 */
void scr_cmd_caption(const char *title);

/* 清空展示区显示 */
void clr_disp();

/* 清空命令区显示 */
void clr_cmd();

/* 设置显示区字符 */
void disp_char(uint8_t x, uint8_t y, char ch);

/* 设置显示区字符 */
void disp_char2(uint16_t pos, char ch);

/* 设置显示区字符属性 */
void disp_attrib(uint8_t x, uint8_t y, uint8_t attrib);

/* 设置显示区字符属性 */
void disp_attrib2(uint16_t pos, uint8_t attrib);

/* disp区滚屏一行 */
void disp_roll_screen();

/* 设置控制区区字符 */
void cmd_char(uint8_t x, uint8_t y, char ch);

/* 设置控制区字符 */
void cmd_char2(uint16_t pos, char ch);

/* 设置控制区字符属性 */
void cmd_attrib(uint8_t x, uint8_t y, uint8_t attrib);

/* 设置控制区字符属性 */
void cmd_attrib2(uint16_t pos, uint8_t attrib);

/* 在显示区显示一个字符串，到行尾自动截断 */
void disp_show_str(uint8_t x, uint8_t y, const char *str);

/* 将屏幕内容拷贝到一个进程的显示缓冲区 */
void copy_scr_to_con_buf(struct PCB *pcb);

/* 将一个进程后台缓冲区的内容拷贝到屏幕 */
void copy_con_buf_to_scr(struct PCB *pcb);

#endif /* TINY_OS_EXPLORER_SCREEN_H */
