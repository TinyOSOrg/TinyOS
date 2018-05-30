#include <kernel/assert.h>
#include <kernel/execelf/execelf.h>
#include <kernel/execelf/readelf.h>
#include <kernel/interrupt.h>
#include <kernel/memory.h>
#include <kernel/process/process.h>

#include <shared/string.h>
#include <shared/sys.h>

/*
    exec要完成以下事情（顺序无先后）：
        创建进程
        进程参数填充
        装载进程代码
        
        步骤：
        1. 关中断
        2. 创建一个虚拟地址空间A，把自己的虚拟地址空间临时切换到A，没错这里需要改PCB，这个耦合我暂时想不到怎么去掉
            因为等会儿读文件的时候会有进程切换，如果不改PCB，等切换回来的时候虚拟地址空间就不对了
        3. 把elf文件内容装载好，把入口地址和参数都放到特定地址，注意这里读文件会执行调度器
        4. 以A创建一个进程，执行入口是elf的入口地址
        5. 把自己的PCB中的虚拟地址空间换回来
        6. 恢复原中断状态

        elf入口函数自己去把命令行参数从特定地址取出来
*/

/*
    进程初始虚拟地址空间规划：
    addr space
        || 低地址区域：
        ||     0~USER_DISPLAYING_BUFFER_ADDR：空闲
        ||     USER_DISPLAYING_BUFFER_ADDR~(USER_STACK_BITMAP_ADDR - 1)：进程屏幕缓存
        ||     屏幕缓存之后紧跟调用栈分配位图，之后的空间均为空闲
        || 高地址区域：
        ||     elf文件缓存
        ||     进程参数区
        \/     调用栈
*/

/* 进程参数区起始地址 */
#define PROC_ARG_ZONE_ADDR \
    (USER_STACK_TOP_ADDR - MAX_PROCESS_THREADS * USER_STACK_SIZE - \
     EXEC_ELF_ARG_MAX_COUNT * EXEC_ELF_ARG_BUF_SIZE - sizeof(uint32_t))
    
/* elf文件缓存区大小 */
#define PROC_FILE_BUF_SIZE (100 * 1024 * 1024)

/* elf文件缓存区起始地址 */
#define PROC_FILE_BUF_ADDR (PROC_ARG_ZONE_ADDR - PROC_FILE_BUF_SIZE)

#define EXEC_ELF_EXIT(rt) \
    do { \
        pcb->addr_space = ori_addr_space; \
        set_current_vir_addr_space(ori_addr_space); \
        set_intr_state(is); \
        return (rt); \
    } while(0)

#define EXEC_ELF_ERROR(rt) \
    do { \
        destroy_vir_addr_space(new_addr_space); \
        EXEC_ELF_EXIT(rt); \
    } while(0)

enum exec_elf_result exec_elf(const char *proc_name,
                              filesys_dp_handle dp, const char *elf_path,
                              bool is_PL_0, uint32_t argc, const char **argv,
                              uint32_t *_pid)
{
    intr_state is = fetch_and_disable_intr();
    struct PCB *pcb = get_cur_PCB();

    ASSERT_S(argc < EXEC_ELF_ARG_MAX_COUNT);

    // 备份原虚拟地址空间，设置新的虚拟地址空间
    vir_addr_space *ori_addr_space = pcb->addr_space;
    vir_addr_space *new_addr_space = create_vir_addr_space();
    pcb->addr_space = new_addr_space;
    set_current_vir_addr_space(new_addr_space);

    // 尝试打开elf文件
    usr_file_handle fp;
    if(open_file(dp, elf_path, false, &fp) != filesys_opr_success)
        EXEC_ELF_ERROR(exec_elf_file_error);

    // 检查文件大小是否越界
    uint32_t file_size = get_file_size(fp);
    if(file_size > PROC_FILE_BUF_SIZE)
    {
        close_file(fp);
        EXEC_ELF_ERROR(exec_elf_file_error);
    }
    
    // 读取文件缓存
    if(read_file(fp, 0, file_size, (void*)PROC_FILE_BUF_ADDR)
        != filesys_opr_success)
    {
        close_file(fp);
        EXEC_ELF_ERROR(exec_elf_file_error);
    }

    close_file(fp);

    // 装载elf
    void *entry_addr = load_elf((const void*)PROC_FILE_BUF_ADDR);
    if(!entry_addr)
        EXEC_ELF_ERROR(exec_elf_invalid_elf);
    
    // 拷贝参数到缓存区域
    char *p_buf = (char*)PROC_ARG_ZONE_ADDR;
    *(uint32_t*)p_buf = argc;
    p_buf += sizeof(uint32_t);
    for(uint32_t i = 0; i < argc; ++i)
    {
        strcpy_s(p_buf, argv[i], EXEC_ELF_ARG_BUF_SIZE);
        p_buf += EXEC_ELF_ARG_BUF_SIZE;
    }

    // 进程创建
    uint32_t pid = create_process_with_addr_space(proc_name, (process_exec_func)entry_addr,
                                                  new_addr_space, is_PL_0);
    if(_pid)
        *_pid = pid;

    EXEC_ELF_EXIT(exec_elf_success);
}

#undef EXEC_ELF_ERROR
#undef EXEC_ELF_EXIT
