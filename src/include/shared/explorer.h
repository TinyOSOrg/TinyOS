#ifndef TINY_OS_SHARED_EXPLORER_H
#define TINY_OS_SHARED_EXPLORER_H

#include <shared/sysmsg.h>
#include <shared/utility.h>

struct expl_output_msg
{
    sysmsg_type type; // 应为SYSMSG_TYPE_EXPL_OUTPUT
    char ch;
    char _padding[3];
    uint32_t out_pid;
    uint32_t out_uid;
};

struct expl_input_msg
{
    sysmsg_type type; // 应为SYSMSG_TYPE_EXPL_INPUT
    char params[SYSMSG_PARAM_SIZE];
};

STATIC_ASSERT(sizeof(struct expl_output_msg) == sizeof(struct sysmsg),
              invalid_size_of_expl_output_msg);

#endif /* TINY_OS_SHARED_EXPLORER_H */
