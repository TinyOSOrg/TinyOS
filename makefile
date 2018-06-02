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

C_SUFFIX = _C_FILES
CPP_SUFFIX = _CPP_FILES
O_SUFFIX = _O_FILES
D_SUFFIX = _D_FILES

TOOLS =
TOOLS_TGTS =

APPS =
APPS_TGTS =

include ./make/tool
include ./make/application

# disk img
HD = hd.img

.PHONY : all
all : $(HD) apps tools

include ./make/lib
include ./make/shared
include ./make/kernel

$(eval $(call make_tool,mkdpt,MKDPT))
$(eval $(call make_tool,bin_trans,BIN_TRANS))
$(eval $(call make_tool,disk_ipt,DISK_IPT))

$(eval $(call make_app,elf_tester,ELF_TESTER))
$(eval $(call make_app,cp,CP))
$(eval $(call make_app,ls,LS))
$(eval $(call make_app,pwd,PWD))
$(eval $(call make_app,cat,CAT))
$(eval $(call make_app,ed,ED))

.PHONY : apps
apps : $(APPS_TGTS)

.PHONY : tools
tools : $(TOOLS_TGTS)

.PHONY: clean
clean :
	rm -f $(shell find ./src/ -name "*.dtmp")
	rm -f $(shell find ./src/ -name "*.o")
	rm -f $(shell find ./src/ -name "*.d")
	rm -f $(shell find ./src/ -name "*.bootbin")
	rm -f $(shell find ./src/ -name "*.bin")
	rm -f $(TOOLS) $(APPS)

.PHONY: run
run :
	make all
	echo c | bochs

.PHONY: fs
fs :
	bash setup_old.sh
	make clean
