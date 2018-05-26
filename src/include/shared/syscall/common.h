#ifndef TINY_OS_SHARED_SYSCALL_COMMON_H
#define TINY_OS_SHARED_SYSCALL_COMMON_H

/* 系统调用入口数量 */
#define SYSCALL_COUNT 15

/* 一个合法的系统调用应返回void或uint32_t，有0~3个uint32_t大小的参数 */

/*
    取得当前进程ID
    uint32_t impl();
*/
#define SYSCALL_GET_PROCESS_ID 0

#define SYSCALL_YIELD_CPU (SYSCALL_GET_PROCESS_ID + 1)

#define SYSCALL_EXIT_THREAD (SYSCALL_YIELD_CPU + 1)

/*
    控制台操作
    func和arg含义见 kernel/console/console.h
    uint32_t impl(uint32_t func, uint32_t arg);
*/
#define SYSCALL_CONSOLE_OPERATION (SYSCALL_EXIT_THREAD + 1)

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

/* 文件系统相关 */

#define SYSCALL_FILESYS_OPEN  (SYSCALL_KEYBOARD_QUERY + 1)
#define SYSCALL_FILESYS_CLOSE (SYSCALL_FILESYS_OPEN + 1)

#define SYSCALL_FILESYS_MKFILE (SYSCALL_FILESYS_CLOSE + 1)
#define SYSCALL_FILESYS_RMFILE (SYSCALL_FILESYS_MKFILE + 1)

#define SYSCALL_FILESYS_MKDIR (SYSCALL_FILESYS_RMFILE + 1)
#define SYSCALL_FILESYS_RMDIR (SYSCALL_FILESYS_MKDIR + 1)

#define SYSCALL_FILESYS_GET_SIZE (SYSCALL_FILESYS_RMDIR + 1)

#define SYSCALL_FILESYS_READ  (SYSCALL_FILESYS_GET_SIZE + 1)
#define SYSCALL_FILESYS_WRITE (SYSCALL_FILESYS_READ + 1)

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

#endif /* TINY_OS_SHARED_SYSCALL_COMMON_H */
