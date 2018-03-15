# 将MBR写入初始引导扇区
hd60M.img : build/mbr.bin
	dd if=build/mbr.bin of=hd60M.img bs=512 count=1 conv=notrunc

# MBR编译
build/mbr.bin : src/boot/mbr.s
	nasm src/boot/mbr.s -o build/mbr.bin

clear :
	rm build/mbr.bin
