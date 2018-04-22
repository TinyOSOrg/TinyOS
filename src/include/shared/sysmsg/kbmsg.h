#ifndef TINY_OS_SHARED_KBMSG_H
#define TINY_OS_SHARED_KBMSG_H

#include <shared/intdef.h>
#include <shared/sysmsg/common.h>

/*
    msg参数含义:
    struct
    {
        uint32_t key
        uint32_t flags
        uint32_t reserved
    }

    flags0 down/up
*/
struct kbmsg_struct
{
    sysmsg_type type; // 为SYSMSG_TYPE_KEYBOARD
    uint32_t key;
    uint32_t flags;
    uint32_t reserved;
};

/*
    char消息
    struct
    {
        uint32_t char
    }
*/
struct kbchar_msg_struct
{
    sysmsg_type type; // 为SYSMSG_TYPE_CHAR
    uint32_t ch;
    uint32_t reserved1;
    uint32_t reserved2;
};

#define KBMSG_FLAG_UP 0x1

#endif /* TINY_OS_SHARED_KBMSG_H */
