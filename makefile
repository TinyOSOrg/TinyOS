CC = gcc
CC_FLAGS = -m32 -std=c99 -nostdinc -I src/include/

LD = ld
LD_FLAGS = -m elf_i386

ASM = nasm
ASM_FLAGS =

BIN_FILE = src/boot/mbr.bin src/boot/bootloader.bin src/boot/kernel.bin

C_SRC_FILES = $(shell find ./src/ -name "*.c")
C_OBJ_FILES = $(patsubst %.c, %.o, $(C_SRC_FILES))
C_DPT_FILES = $(patsubst %.c, %.d, $(C_SRC_FILES))

hd60M.img : $(BIN_FILE)
	dd if=src/boot/mbr.bin of=hd60M.img bs=512 count=1 conv=notrunc
	dd if=src/boot/bootloader.bin of=hd60M.img bs=512 count=4 seek=1 conv=notrunc
	dd if=src/boot/kernel.bin of=hd60M.img bs=512 count=200 seek=9 conv=notrunc

# MBR编译
src/boot/mbr.bin : src/boot/mbr.s src/boot/boot.s
	$(ASM) src/boot/mbr.s -o src/boot/mbr.bin

# bootloader
src/boot/bootloader.bin : src/boot/bootloader.s src/boot/boot.s
	$(ASM) src/boot/bootloader.s -o src/boot/bootloader.bin

src/boot/kernel.bin : $(C_OBJ_FILES)
	$(LD) $(LD_FLAGS) $(C_OBJ_FILES) -Ttext 0xc0001200 -e main -o src/boot/kernel.bin

.c.o:
	$(CC) $(CC_FLAGS) -c $< -o $@

# 头文件依赖
%.d:%.c
	@set -e; rm -f $@; $(CC) -MM $< -I src/include/ > $@.$$$$; \
	sed 's,\($*\)\.o[ :]*,\1.o $@ : ,g' < $@.$$$$ > $@; \
	rm -f $@.$$$$

-include $(C_OBJ_FILES:.o=.d)

clean :
	rm -f $(BIN_FILE) $(C_OBJ_FILES) $(C_DPT_FILES)

bochs :
	make
	bochs
