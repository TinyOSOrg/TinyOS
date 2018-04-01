CC = gcc
CC_INCLUDE_FLAGS = -I src/include/
CC_FLAGS = -m32 -nostdinc $(CC_INCLUDE_FLAGS) -fno-stack-protector

LD = ld
LD_FLAGS = -m elf_i386

ASM = nasm
ASM_FLAGS =

BOOTBIN_FILE = src/boot/mbr.bootbin src/boot/bootloader.bootbin src/boot/kernel.bootbin

C_SRC_FILES = $(shell find ./src/ -name "*.c")
C_OBJ_FILES = $(patsubst %.c, %.o, $(C_SRC_FILES))
C_DPT_FILES = $(patsubst %.c, %.d, $(C_SRC_FILES))

S_SRC_FILES = $(shell find ./src/kernel/ -name "*.s")
S_BIN_FILES = $(patsubst %.s, %.bin, $(S_SRC_FILES))

hd60M.img : $(BOOTBIN_FILE)
	dd if=src/boot/mbr.bootbin of=hd60M.img bs=512 count=1 conv=notrunc
	dd if=src/boot/bootloader.bootbin of=hd60M.img bs=512 count=4 seek=1 conv=notrunc
	dd if=src/boot/kernel.bootbin of=hd60M.img bs=512 count=200 seek=9 conv=notrunc

# MBR编译
src/boot/mbr.bootbin : src/boot/mbr.s src/boot/boot.s
	$(ASM) $(ASM_FLAGS) $< -o $@

# bootloader
src/boot/bootloader.bootbin : src/boot/bootloader.s src/boot/boot.s
	$(ASM) $(ASM_FLAGS) $< -o $@

src/boot/kernel.bootbin : $(C_OBJ_FILES) $(S_BIN_FILES)
	$(LD) $(LD_FLAGS) $(C_OBJ_FILES) $(S_BIN_FILES) -Ttext 0xc0002000 -e main -o $@

%.o : %.c
	$(CC) $(CC_FLAGS) -c $< -o $@

%.bin : %.s
	$(ASM) $(ASM_FLAGS) -f elf $< -o $@

# 头文件依赖
%.d : %.c
	@set -e; rm -f $@; $(CC) -MM $< $(CC_INCLUDE_FLAGS) > $@.$$$$; \
	sed 's,\($*\)\.o[ :]*,\1.o $@ : ,g' < $@.$$$$ > $@; \
	rm -f $@.$$$$

-include $(C_OBJ_FILES:.o=.d)

clean :
	rm -f $(BOOTBIN_FILE) $(C_OBJ_FILES) $(C_DPT_FILES) $(S_BIN_FILES)

bochs :
	make
	bochs
