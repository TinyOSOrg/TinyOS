hd60M.img : build/mbr.bin
	dd if=build/mbr.bin of=hd60M.img bs=512 count=1 conv=notrunc
build/mbr.bin : src/boot/mbr.s
	nasm src/boot/mbr.s -o build/mbr.bin
clear :
	rm build/mbr.bin
