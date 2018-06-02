rm -rf build
rm -f hd.img
bximage -mode=create -hd=128M -imgmode=flat -q hd.img
mkdir build
make clean tools apps
./build/mkdpt hd.img
./build/disk_ipt hd.img ./build/cp         /apps/cp \
                        ./build/ls         /apps/ls \
                        ./build/pwd        /apps/pwd \
                        ./build/cat        /apps/cat \
                        ./build/ed         /apps/ed \
                        ./build/mf         /apps/mf \
                        ./makefile         /makefile
