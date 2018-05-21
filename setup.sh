rm -rf build
rm -rf hd.img
bximage -mode=create -hd=128M -imgmode=flat -q hd.img
mkdir build
make mkdpt
./build/mkdpt hd.img
make clean
