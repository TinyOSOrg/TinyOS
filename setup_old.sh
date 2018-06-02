rm -rf build
rm -f hd.img
bximage -hd -size=128M -mode=flat -q hd.img
mkdir build
make clean tools apps
./build/mkdpt hd.img
./build/disk_ipt hd.img ./build/elf_tester /test.elf \
                        ./build/cp         /apps/cp \
                        ./build/ls         /apps/ls \
                        ./build/pwd        /apps/pwd \
                        ./build/cat        /apps/cat \
                        ./build/ed         /apps/ed \
                        ./makefile         /makefile
