#ifndef TINY_OS_BOOT_H
#define TINY_OS_BOOT_H

/* 总内存容量由bootloader读取，被放在这 */
#define TOTAL_MEMORY_SIZE_ADDR 0x500

/* 内核栈末尾（物理） */
#define KER_STACK_PHY_END 0x200000

/* 内核页目录开头（物理） */
#define KER_PDE_PHY_ADDR 0x300000

#define KER_PDE_VIR_ADDR (0xc0000000 + KER_PDE_PHY_ADDR)

#endif /* TINY_OS_BOOT_H */
