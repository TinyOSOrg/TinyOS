#include <shared/explorer.h>
#include <shared/string.h>
#include <shared/sysmsg.h>
#include <shared/sys.h>

#include <lib/input.h>

static char input_buf[SYSMSG_PARAM_SIZE + 1];

static uint32_t input_buf_idx;

void _init_input()
{
    memset(input_buf, 0x0, SYSMSG_PARAM_SIZE);
    input_buf_idx = 0;
}

char get_char()
{
    if(input_buf[input_buf_idx])
        return input_buf[input_buf_idx++];

    while(true)
    {
        wait_for_sysmsg();
        struct sysmsg msg;
        if(peek_sysmsg(SYSMSG_SYSCALL_PEEK_OPERATION_REMOVE, &msg) &&
           msg.type == SYSMSG_TYPE_EXPL_INPUT)
        {
            memcpy(input_buf, (char*)msg.params, SYSMSG_PARAM_SIZE);
            input_buf_idx = 0;
            break;
        }
    }

    return input_buf[input_buf_idx++];
}
