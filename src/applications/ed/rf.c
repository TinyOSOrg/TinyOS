#include <shared/sys.h>
#include <shared/utility.h>

#include "rf.h"

#define BUF_SIZE 128

static usr_file_handle fp;
static uint32_t fpos, fsize;

static char buf[BUF_SIZE];
static uint32_t buf_idx;

void rf_init(usr_file_handle file)
{
    fp    = file;
    fpos  = 0;
    fsize = get_file_size(file);

    buf_idx = BUF_SIZE;
}

char rf_provider()
{
    if(buf_idx >= BUF_SIZE)
    {
        if(fpos >= fsize)
            return '\0';

        uint32_t delta  = MIN(BUF_SIZE, fsize - fpos);
        uint32_t offset = BUF_SIZE - delta;
        read_file(fp, fpos, delta, buf + offset);
        fpos += delta;
        buf_idx = offset;
    }

    return buf[buf_idx++];
}
