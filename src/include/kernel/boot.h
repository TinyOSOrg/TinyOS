#ifndef TINY_OS_BOOT_H
#define TINY_OS_BOOT_H

//总内存容量由bootloader读取，被放在这
#define TOTAL_MEMORY_SIZE_ADDR 0x500

//内核保留的低地址区域末尾
#define KERNEL_RESERVED_PHY_MEM_END 0x300000

#endif //TINY_OS_BOOT_H
