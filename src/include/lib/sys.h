#ifndef TINY_OS_LIB_SYS_H
#define TINY_OS_LIB_SYS_H

#include <shared/filesys/filesys.h>
#include <shared/keycode.h>
#include <shared/syscall/filesys.h>
#include <shared/syscall/sysmsg.h>
#include <shared/sysmsg/common.h>
#include <shared/sysmsg/kbmsg.h>

#include <lib/stdbool.h>
#include <lib/stdint.h>

/* 进程/线程相关 */

uint32_t get_pid();

void yield_cpu();

void exit_thread();

/* 屏幕输出相关 */

void set_char_row_col(uint8_t row, uint8_t col, char ch);

void set_char_at(uint16_t pos, char ch);

void set_char_attrib_row_col(uint8_t row, uint8_t col, uint8_t attrib);

void get_cursor_row_col(uint8_t *row, uint8_t *col);

void set_cursor_row_col(uint8_t row, uint8_t col);

char get_char_row_col(uint8_t row, uint8_t col);

void put_char(char ch);

void put_str(const char *str);

void printf(const char *fmt, ...);

void roll_scr(uint32_t beg_row, uint32_t end_row);

/* 文件系统相关 */

enum filesys_opr_result open_file(filesys_dp_handle dp,
                                  const char *path, bool writing,
                                  usr_file_handle *result);

enum filesys_opr_result close_file(usr_file_handle file);

enum filesys_opr_result make_file(filesys_dp_handle dp,
                                  const char *path);

enum filesys_opr_result remove_file(filesys_dp_handle dp,
                                    const char *path);

enum filesys_opr_result make_directory(filesys_dp_handle dp,
                                       const char *path);

enum filesys_opr_result remove_directory(filesys_dp_handle dp,
                                         const char *path);

uint32_t get_file_size(usr_file_handle file);

enum filesys_opr_result write_file(usr_file_handle file,
                                  uint32_t fpos, uint32_t byte_size,
                                  const void *data);

enum filesys_opr_result read_file(usr_file_handle file,
                                  uint32_t fpos, uint32_t byte_size,
                                  void *data);

enum filesys_opr_result get_child_file_count(filesys_dp_handle dp,
                                             const char *path,
                                             uint32_t *rt);

enum filesys_opr_result get_child_file_info(filesys_dp_handle dp,
                                            const char *path,
                                            uint32_t idx,
                                            struct syscall_filesys_file_info *rt);

/* 键盘状态与消息相关 */

/* 某个给定的按键是否处于按压状态 */
bool is_key_pressed(uint8_t kc);

/* 注册按键消息 */
void register_key_msg();

/* 注册字符消息 */
void register_char_msg();

#define is_kbmsg_down(MSG_PTR) \
    ((((struct kbmsg_struct*)(MSG_PTR))->flags & KBMSG_FLAG_UP) == 0)

#define is_kbmsg_up(MSG_PTR) (!is_kbmsg_down(MSG_PTR))

#define get_kbmsg_key(MSG_PTR) ((uint8_t)((struct kbmsg_struct*)(MSG_PTR))->key)

#define get_chmsg_char(MSG_PTR) ((char)((struct kbchar_msg_struct*)(MSG_PTR))->ch)

/* 消息队列相关 */

/* 查询消息队列是否不为空 */
bool has_sysmsg();

/* 尝试取走一条消息，opr见shared/sysmsg/common，无消息时返回false */
bool peek_sysmsg(uint32_t opr, struct sysmsg *msg);

/* 阻塞自己，直到一条消息到来 */
void wait_for_sysmsg();

/* 清空消息队列 */
void clr_sysmsgs();

#endif /* TINY_OS_LIB_SYS_H */
