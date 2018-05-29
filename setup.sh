rm -rf build
rm -f hd.img
bximage -mode=create -hd=128M -imgmode=flat -q hd.img
mkdir build
make mkdpt
make elf_tester
make disk_ipt
./build/mkdpt hd.img
./build/disk_ipt hd.img ./build/elf_tester /test.elf
make clean
