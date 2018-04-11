#ifndef TINY_OS_BOOT_H
#define TINY_OS_BOOT_H

/* 总内存容量由bootloader读取，被放在这 */
#define TOTAL_MEMORY_SIZE_ADDR 0x500

/* 内核栈末尾（物理） */
#define KER_STACK_PHY_END 0x200000

/* 内核栈的起始值（虚拟） */
#define KER_STACK_INIT_VAL 0xc01ff000

/* 内核页目录开头（物理） */
#define KER_PDE_PHY_ADDR 0x200000

/* 内核页目录地址（虚拟） */
#define KER_PDE_VIR_ADDR (0xc0000000 + KER_PDE_PHY_ADDR)

/* 静态内核存储区开头 */
#define STATIC_KERNEL_MEM_START 0xc0300000

/*
    静态内核存储区大小
    应是0x1000的整数倍
*/
#define STATIC_KERNEL_MEM_SIZE 0x100000

/* bootloader在虚拟内存中的位置 */
#define BOOTLOADER_START_ADDR 0xc0000900

/* 全局描述符表在虚存中的位置 */
#define GDT_START (BOOTLOADER_START_ADDR + 3)

#endif /* TINY_OS_BOOT_H */
