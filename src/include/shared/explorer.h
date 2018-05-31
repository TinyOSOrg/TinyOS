#ifndef TINY_OS_SHARED_EXPLORER_H
#define TINY_OS_SHARED_EXPLORER_H

#include <shared/sysmsg.h>

struct expl_input_msg
{
    sysmsg_type type; // 应为SYSMSG_TYPE_EXPL_INPUT
    char params[SYSMSG_PARAM_SIZE];
};

#endif /* TINY_OS_SHARED_EXPLORER_H */
