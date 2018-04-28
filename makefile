CC = gcc
CC_INCLUDE_FLAGS = -I src/include/
CC_FLAGS = -m32 -nostdinc $(CC_INCLUDE_FLAGS) \
		   -fno-stack-protector -std=gnu99 \
		   -fno-builtin -Werror -Wall -O2

LD = ld
LD_FLAGS = -m elf_i386

ASM = nasm
ASM_FLAGS =

BOOTBIN_FILE = src/boot/mbr.bootbin src/boot/bootloader.bootbin src/boot/kernel.bootbin

KER_C_FILES =  $(shell find ./src/kernel/ -name "*.c")
KER_C_FILES += $(shell find ./src/shared/ -name "*.c")

KER_O_FILES = $(patsubst %.c, %.o, $(KER_C_FILES))
KER_D_FILES = $(patsubst %.c, %.d, $(KER_C_FILES))

KER_S_FILES =  $(shell find ./src/kernel/ -name "*.s")
KER_S_FILES += $(shell find ./src/shared/ -name "*.s")

KER_BIN_FILES = $(patsubst %.s, %.bin, $(KER_S_FILES))

KER_ENTRY_ADDR = 0xc0002000

HD = hd.img

$(HD) : $(BOOTBIN_FILE)
	dd if=src/boot/mbr.bootbin of=$(HD) bs=512 count=1 conv=notrunc
	dd if=src/boot/bootloader.bootbin of=$(HD) bs=512 count=4 seek=1 conv=notrunc
	dd if=src/boot/kernel.bootbin of=$(HD) bs=512 count=200 seek=9 conv=notrunc

# MBR编译
src/boot/mbr.bootbin : src/boot/mbr.s src/boot/boot.s
	$(ASM) $(ASM_FLAGS) $< -o $@

# bootloader
src/boot/bootloader.bootbin : src/boot/bootloader.s src/boot/boot.s
	$(ASM) $(ASM_FLAGS) $< -o $@

src/boot/kernel.bootbin : $(KER_O_FILES) $(KER_BIN_FILES)
	$(LD) $(LD_FLAGS) $(KER_O_FILES) $(KER_BIN_FILES) -Ttext $(KER_ENTRY_ADDR) -e main -o $@

%.o: %.c
	$(CC) $(CC_FLAGS) -c $< -o $@

%.bin: %.s
	$(ASM) $(ASM_FLAGS) -f elf $< -o $@

# 头文件依赖
%.d: %.c
	@set -e; \
	rm -f $@; \
	$(CC) -MM $(CC_FLAGS) $< $(CC_INCLUDE_FLAGS) > $@.$$$$.dtmp; \
	sed 's,\(.*\)\.o\:,$*\.o $*\.d\:,g' < $@.$$$$.dtmp > $@; \
	rm -f $@.$$$$.dtmp

-include $(KER_D_FILES)

clean :
	rm -f $(BOOTBIN_FILE)
	rm -f $(KER_O_FILES)
	rm -f $(KER_D_FILES) $(shell find ./src/ -name "*.dtmp")
	rm -f $(KER_BIN_FILES)

bochs :
	make
	bochs
