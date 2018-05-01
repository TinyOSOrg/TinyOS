rm -rf tools
rm -rf hd.img
bximage -mode=create -hd=128M -imgmode=flat -q hd.img
mkdir tools
