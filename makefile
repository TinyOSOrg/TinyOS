hd60M.img : build/mbr.bin build/bootloader.bin
	dd if=build/mbr.bin of=hd60M.img bs=512 count=1 conv=notrunc
	dd if=build/bootloader.bin of=hd60M.img bs=512 count=4 seek=2 conv=notrunc

# MBR编译
build/mbr.bin : src/boot/mbr.s src/boot/boot.s
	nasm src/boot/mbr.s -o build/mbr.bin

# bootloader
build/bootloader.bin : src/boot/bootloader.s src/boot/boot.s
	nasm src/boot/bootloader.s -o build/bootloader.bin

clear :
	rm build/mbr.bin

bochs :
	make
	bochs
