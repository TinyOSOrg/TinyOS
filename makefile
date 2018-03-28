CC = gcc
CC_FLAGS = -m32

BIN_FILE = build/mbr.bin build/bootloader.bin build/kernel.bin
OBJ_FILE = build/main.o

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

build/kernel.bin : build/main.o
	ld -m elf_i386 build/main.o -Ttext 0xc0002000 -e main -o build/kernel.bin

build/main.o : src/kernel/main.c
	$(CC) $(CC_FLAGS) -c src/kernel/main.c -o build/main.o

clean :
	rm -f $(BIN_FILE) $(OBJ_FILE)

bochs :
	make
	bochs
