############################## 编译选项 ##############################

CC = gcc
CC_INCLUDE_FLAGS = -nostdinc -I src/include/
CC_FLAGS = -m32 $(CC_INCLUDE_FLAGS) \
		   -fno-stack-protector -std=gnu99 \
		   -fno-builtin -Werror -Wall -O2

CPPC = g++
CPPC_INCLUDE_FLAGS = -I src/include/
CPPC_FLAGS = -std=c++14 -Werror -Wall -O2 $(CPPC_INCLUDE_FLAGS)

LD = ld
LD_FLAGS = -m elf_i386

ASM = nasm
ASM_FLAGS =

############################## 目标 ##############################

# 磁盘映像
HD = hd.img

# 分区表创建工具
MKDPT = tools/mkdpt

.PHONY : all
all : $(HD) tools

.PHONY : mkdpt
mkdpt : $(MKDPT)

.PHONY : tools
tools : mkdpt

############################## 内核 ##############################

# 写入磁盘映像的文件列表
BOOTBIN_FILE = src/boot/mbr.bootbin src/boot/bootloader.bootbin src/boot/kernel.bootbin

KER_C_FILES =  $(shell find ./src/kernel/ -name "*.c")
KER_C_FILES += $(shell find ./src/shared/ -name "*.c")
KER_C_FILES += $(shell find ./src/lib/ -name "*.c")

KER_O_FILES = $(patsubst %.c, %.o, $(KER_C_FILES))
KER_D_FILES = $(patsubst %.c, %.d, $(KER_C_FILES))

KER_S_FILES =  $(shell find ./src/kernel/ -name "*.s")
KER_S_FILES += $(shell find ./src/shared/ -name "*.s")
KER_C_FILES += $(shell find ./src/lib/ -name "*.s")

KER_BIN_FILES = $(patsubst %.s, %.bin, $(KER_S_FILES))

KER_ENTRY_ADDR = 0xc0002000

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

$(KER_O_FILES) : %.o : %.c
	$(CC) $(CC_FLAGS) -c $< -o $@

%.bin : %.s
	$(ASM) $(ASM_FLAGS) -f elf $< -o $@

# 头文件依赖

$(KER_D_FILES) : %.d : %.c
	@set -e; \
	rm -f $@; \
	$(CC) -MM $(CC_FLAGS) $< $(CC_INCLUDE_FLAGS) > $@.$$$$.dtmp; \
	sed 's,\(.*\)\.o\:,$*\.o $*\.d\:,g' < $@.$$$$.dtmp > $@; \
	rm -f $@.$$$$.dtmp

-include $(KER_D_FILES)

############################## 分区表创建工具 ##############################

MKDPT_CPP_FILES = $(shell find ./src/tools/mkdpt/ -name "*.cpp")

MKDPT_O_FILES = $(patsubst %.cpp, %.o, $(MKDPT_CPP_FILES))
MKDPT_D_FILES = $(patsubst %.cpp, %.d, $(MKDPT_CPP_FILES))

$(MKDPT) : $(MKDPT_O_FILES)
	$(CPPC) $< -o $@

$(MKDPT_O_FILES) : %.o : %.cpp
	$(CPPC) $(CPPC_FLAGS) -c $< -o $@

$(MKDPT_D_FILES) : %.d : %.cpp
	@set -e; \
	rm -f $@; \
	$(CPPC) -MM $(CPPC_FLAGS) $< $(CPPC_INCLUDE_FLAGS) > $@.$$$$.dtmp; \
	sed 's,\(.*\)\.o\:,$*\.o $*\.d\:,g' < $@.$$$$.dtmp > $@; \
	rm -f $@.$$$$.dtmp

-include $(MKDPT_D_FILES)

############################## make选项 ##############################

.PHONY: clean
clean :
	rm -f $(BOOTBIN_FILE)

	rm -f $(shell find ./src/ -name "*.dtmp")

	rm -f $(KER_O_FILES)
	rm -f $(KER_D_FILES)
	rm -f $(KER_BIN_FILES)

	rm -f $(MKDPT)
	rm -f $(MKDPT_O_FILES)
	rm -f $(MKDPT_D_FILES)

.PHONY: bochs
bochs :
	make all
	bochs
