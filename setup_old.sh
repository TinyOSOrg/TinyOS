rm -rf build
rm -f hd.img
bximage -hd -size=128M -mode=flat -q hd.img
mkdir build
make clean tools apps
./build/mkdpt hd.img
./build/disk_ipt hd.img ./build/cp                      /apps/cp \
                        ./build/ls                      /apps/ls \
                        ./build/pwd                     /apps/pwd \
                        ./build/cat                     /apps/cat \
                        ./build/ed                      /apps/ed \
                        ./build/file                    /apps/file \
                        ./build/less                    /apps/less \
                        ./build/help                    /apps/help \
                        ./src/docs/cat.txt              /docs/cat \
                        ./src/docs/cd.txt               /docs/cd \
                        ./src/docs/clear.txt            /docs/clear \
                        ./src/docs/cp.txt               /docs/cp \
                        ./src/docs/ed.txt               /docs/ed \
                        ./src/docs/exec.txt             /docs/exec \
                        ./src/docs/exit.txt             /docs/exit \
                        ./src/docs/fg.txt               /docs/fg \
                        ./src/docs/file.txt             /docs/file \
                        ./src/docs/help.txt             /docs/help \
                        ./src/docs/kill.txt             /docs/kill \
                        ./src/docs/less.txt             /docs/less \
                        ./src/docs/ls.txt               /docs/ls \
                        ./src/docs/ps.txt               /docs/ps \
                        ./src/docs/pwd.txt              /docs/pwd \
                        ./makefile                      /makefile
