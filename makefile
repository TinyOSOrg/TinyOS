CC = gcc
CC_FLAGS = -m32

LD = ld
LD_FLAGS = -m elf_i386

BIN_FILE = build/mbr.bin build/bootloader.bin build/kernel.bin
OBJ_FILE = build/main.o build/print.o

hd60M.img : $(BIN_FILE)
	dd if=build/mbr.bin of=hd60M.img bs=512 count=1 conv=notrunc
	dd if=build/bootloader.bin of=hd60M.img bs=512 count=4 seek=1 conv=notrunc
	dd if=build/kernel.bin of=hd60M.img bs=512 count=200 seek=9 conv=notrunc

# MBR编译
build/mbr.bin : src/boot/mbr.s src/boot/boot.s
	nasm src/boot/mbr.s -o build/mbr.bin

# bootloader
build/bootloader.bin : src/boot/bootloader.s src/boot/boot.s
	nasm src/boot/bootloader.s -o build/bootloader.bin

build/kernel.bin : $(OBJ_FILE)
	# -Ttext参数和boot.s中的KERNEL_ENTRY对应
	$(LD) $(LD_FLAGS) $(OBJ_FILE) -Ttext 0xc0002000 -e main -o build/kernel.bin

build/main.o : src/kernel/main.c
	$(CC) $(CC_FLAGS) -c src/kernel/main.c -o build/main.o

build/print.o : src/kernel/print.c
	$(CC) $(CC_FLAGS) -c src/kernel/print.c -o build/print.o

clean :
	rm -f $(BIN_FILE) $(OBJ_FILE)

bochs :
	make
	bochs
