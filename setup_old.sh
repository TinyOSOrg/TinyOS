rm -rf build
rm -f hd.img
bximage -hd -size=128M -mode=flat -q hd.img
mkdir build
make tools applications
./build/mkdpt hd.img
./build/disk_ipt hd.img ./build/elf_tester /test.elf ./build/cp /cp
make clean
