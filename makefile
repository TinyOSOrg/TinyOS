#=========================== 编译链接选项 ===========================

CC = gcc
CC_INCLUDE_FLAGS = -nostdinc -I src/include/
CC_FLAGS = -m32 $(CC_INCLUDE_FLAGS) \
		   -fno-stack-protector -std=gnu99 \
		   -fno-builtin -Werror -Wall -O2

CPPC = g++
CPPC_INCLUDE_FLAGS = -I src/include/ -I .
CPPC_FLAGS = -std=c++14 -Werror -Wall -O2 $(CPPC_INCLUDE_FLAGS)

LD = ld
LD_FLAGS = -m elf_i386

ASM = nasm
ASM_FLAGS =

#=========================== 目标 ===========================

# 磁盘映像
HD = hd.img

# 分区表创建工具
MKDPT = build/mkdpt

# 将二进制文件转换为十进制字面量的工具
BINTRANS = build/bin_trans

# elf装载器测试程序
ELF_TESTER = build/elf_tester

# 应用程序cp
CP = build/cp

# 外部文件导入工具
DISK_IPT = build/disk_ipt

.PHONY : all
all : $(HD) tools applications

.PHONY : mkdpt
mkdpt : $(MKDPT)

.PHONY : bin_trans
bin_trans : $(BINTRANS)

.PHONY : disk_ipt
disk_ipt : $(DISK_IPT)

.PHONY : elf_tester
elf_tester : $(ELF_TESTER)

.PHONY : cp
cp : $(CP)

.PHONY : tools
tools : mkdpt bin_trans disk_ipt

.PHONY : applications
applications : elf_tester cp

#=========================== 用户库 ===========================

include ./make/lib

#=========================== 共享代码 ===========================

include ./make/shared

#=========================== 内核 ===========================

include ./make/kernel

#=========================== 各种工具和应用程序 ===========================

include ./make/tools/mkdpt

include ./make/tools/bin_trans

include ./make/tools/disk_ipt

include ./make/applications/elf_tester

include ./make/applications/cp

#=========================== make选项 ===========================

.PHONY: clean
clean :
	rm -f $(BOOTBIN_FILE)

	rm -f $(shell find ./src/ -name "*.dtmp")

	rm -f $(LIB_O_FILES)
	rm -f $(LIB_D_FILES)

	rm -f $(SHARED_O_FILES)
	rm -f $(SHARED_D_FILES)

	rm -f $(KER_O_FILES)
	rm -f $(KER_D_FILES)
	rm -f $(KER_BIN_FILES)

	rm -f $(MKDPT)
	rm -f $(MKDPT_O_FILES)
	rm -f $(MKDPT_D_FILES)

	rm -f $(BINTRANS)
	rm -f $(BINTRANS_O_FILES)
	rm -f $(BINTRANS_D_FILES)

	rm -f $(DISK_IPT)
	rm -f $(DISK_IPT_O_FILES)
	rm -f $(DISK_IPT_D_FILES)

	rm -f $(ELF_TESTER)
	rm -f $(ELF_O_FILES)
	rm -f $(ELF_D_FILES)

	rm -f $(CP)
	rm -f $(CP_O_FILES)
	rm -f $(CP_D_FILES)

.PHONY: bochs
bochs :
	make all
	bochs

.PHONY: setup
setup :
	bash setup_old.sh
