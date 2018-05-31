#ifndef TINY_OS_LIB_INPUT_H
#define TINY_OS_LIB_INPUT_H

void _init_input();

/*
    阻塞地从explorer处获取一个字符
    在进程自身无显示缓存时有效，否则该函数不会返回
*/
char get_char();

#endif /* TINY_OS_LIB_INPUT_H */
