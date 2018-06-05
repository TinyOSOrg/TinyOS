#ifndef TINY_OS_SHARD_SYSCALL_H
#define TINY_OS_SHARD_SYSCALL_H

/* 系统调用入口数量 */
#define SYSCALL_COUNT 25

/* 一个合法的系统调用应返回void或uint32_t，有0~3个uint32_t大小的参数 */

/*
    取得当前进程ID
    uint32_t impl();
*/
#define SYSCALL_GET_PROCESS_ID 0

/* 暂时让出CPU让别的线程执行，无参数无返回值 */
#define SYSCALL_YIELD_CPU (SYSCALL_GET_PROCESS_ID + 1)

/* 干掉自己所处的线程，无参数无返回值 */
#define SYSCALL_EXIT_THREAD (SYSCALL_YIELD_CPU + 1)

/*
    在本进程中创建一个新线程
    uint32_t impl(void(*entry)());
    返回false时创建失败
*/
#define SYSCALL_NEW_THREAD (SYSCALL_EXIT_THREAD + 1)

/*
    控制台操作
    func和arg含义见 kernel/console/console.h
    uint32_t impl(uint32_t func, uint32_t arg);
*/
#define SYSCALL_CONSOLE_OPERATION (SYSCALL_NEW_THREAD + 1)

/*
    系统消息队列操作
    参数含义见 kernel/sysmsg/proc_sysmsg_syscall.h
    uint32_t impl(uint32_t func, uint32_t arg1, uint32_t arg2);
*/
#define SYSCALL_SYSMSG_OPERATION (SYSCALL_CONSOLE_OPERATION + 1)

/*
    键盘查询
    参数含义见kernel/kbdriver.h
    uint32_t impl(uint32_t func, uint32_t arg);
*/
#define SYSCALL_KEYBOARD_QUERY (SYSCALL_SYSMSG_OPERATION + 1)

/* 文件系统相关，参数见kernel/filesys/syscall.h */

#define SYSCALL_FILESYS_OPEN  (SYSCALL_KEYBOARD_QUERY + 1)
#define SYSCALL_FILESYS_CLOSE (SYSCALL_FILESYS_OPEN + 1)

#define SYSCALL_FILESYS_MKFILE (SYSCALL_FILESYS_CLOSE + 1)
#define SYSCALL_FILESYS_RMFILE (SYSCALL_FILESYS_MKFILE + 1)

#define SYSCALL_FILESYS_MKDIR (SYSCALL_FILESYS_RMFILE + 1)
#define SYSCALL_FILESYS_RMDIR (SYSCALL_FILESYS_MKDIR + 1)

#define SYSCALL_FILESYS_GET_SIZE (SYSCALL_FILESYS_RMDIR + 1)

#define SYSCALL_FILESYS_READ  (SYSCALL_FILESYS_GET_SIZE + 1)
#define SYSCALL_FILESYS_WRITE (SYSCALL_FILESYS_READ + 1)

#define SYSCALL_FILESYS_GET_CHILD_COUNT (SYSCALL_FILESYS_WRITE + 1)

#define SYSCALL_FILESYS_GET_CHILD_INFO (SYSCALL_FILESYS_GET_CHILD_COUNT + 1)

#define SYSCALL_DP_GET_HANDLE (SYSCALL_FILESYS_GET_CHILD_INFO + 1)

/*
    申请称为当前的前台进程，无参数
    成功时返回true；若此时有别的前台进程，则会申请失败，返回false
*/
#define SYSCALL_EXPL_ALLOC_FOREGROUND (SYSCALL_DP_GET_HANDLE + 1)

/*
    把自己切到后台
    若自己本来就不处于前台，返回false；否则切换成功，返回true
*/
#define SYSCALL_EXPL_FREE_FOREGROUND (SYSCALL_EXPL_ALLOC_FOREGROUND + 1)

#define SYSCALL_EXPL_ALLOC_CON_BUF (SYSCALL_EXPL_FREE_FOREGROUND + 1)

/*
    在disp区域输出一个字符
    若调用方有自己的显示缓存，那么什么也不做，直接返回
    若调用方处于前台，那么输出字符后返回
    若调用方处于后台，那么一直等待直到进程被切换到前台后才输出字符并返回

    仅一个参数 uint32_t arg，其最低字节为要输出的字符
*/
#define SYSCALL_EXPL_PUT_CHAR (SYSCALL_EXPL_ALLOC_CON_BUF + 1)

#define SYSCALL_EXPL_NEW_LINE (SYSCALL_EXPL_PUT_CHAR + 1)

#define SYSCALL_PIPE_NULL_CHAR (SYSCALL_EXPL_NEW_LINE + 1)

#define syscall_param0(N) \
    ({ uint32_t r; \
       asm volatile ("int $0x80;" \
                     : "=a" (r) \
                     : "a" (N) \
                     : "memory"); \
       r; })

#define syscall_param1(N, arg1) \
    ({ uint32_t r; \
       asm volatile ("int $0x80" \
                     : "=a" (r) \
                     : "a" (N), "b" (arg1) \
                     : "memory"); \
       r; })

#define syscall_param2(N, arg1, arg2) \
    ({ uint32_t r; \
       asm volatile ("int $0x80" \
                     : "=a" (r) \
                     : "a" (N), "b" (arg1), "c" (arg2) \
                     : "memory"); \
       r; })

#define syscall_param3(N, arg1, arg2, arg3) \
    ({ uint32_t r; \
       asm volatile ("int $0x80" \
                     : "=a" (r) \
                     : "a" (N), "b" (arg1), "c" (arg2), "d" (arg3) \
                     : "memory"); \
       r; })

#endif /* TINY_OS_SHARD_SYSCALL_H */
