# TinyOS

学习用小型操作系统，运行在x86（32位）计算机上。

---

## 特性

- [x] 虚拟存储
- [x] 内核线程
- [x] 信号量
- [x] 进程管理
- [x] 系统调用
- [x] 键盘驱动
- [x] 消息传递
- [x] 硬盘驱动
- [x] 文件系统
- [x] 交互界面

## 编译

bochs的编译运行依赖于：

1. bochs(2.65+)
2. gnu make，gcc (C99)，g++ (C++14)，ld
3. 汇编器nasm
4. 命令行工具dd
5. bximage（通常随bochs一起安装）

要使用bochs运行该系统，可执行以下步骤：

1. 跟据bochs所附带的显示插件，修改`bochsrc.txt`中的`display_library`项。
2. 初次运行前，运行`bash setup.sh`初始化环境。
3. 运行`make bochs`以构建项目并启动虚拟机。

由于磁盘映像创建工具bximage的命令参数格式曾发生变动，因此若无法正确创建磁盘映像文件，可尝试运行`bash setup_old.sh`以解决问题。

## 截图

![ss01](./doc/pics/ss01.png)

## 交互

系统默认交互界面分为上边的Output区域和下边的Command区域，Output区域用于显示输出，Command区域用于输入命令。系统已经提供的命令包括：

1. pwd                : 显示当前工作路径
2. ls                 : 显示当前工作路径下的文件列表
3. cd path            : 修改当前工作路径
4. ps                 : 查看后台运行的进程列表
5. fg pid             : 将一个后台进程切换到前台
6. dp                 : 查看当前所处分区的名字
7. clear              : 清空Output区域
8. exec path name ... : 从一个elf文件创建一个进程
9. exit               : 退出系统
10. mkdir             : 创建一个空目录
11. rmdir             : 删除一个空目录
12. rmfile            : 删除一个常规文件
