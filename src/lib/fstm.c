#include <shared/sys.h>
#include <shared/utility.h>

#include <lib/fstm.h>
#include <lib/mem.h>

ifstm_t *ifstm_init(filesys_dp_handle dp, const char *path,
                    size_t buf_size)
{
    usr_file_handle fp;
    if(open_file(dp, path, false, &fp) != filesys_opr_success)
        return NULL;
    
    ifstm_t *ret = malloc(sizeof(ifstm_t) + buf_size);
    if(!ret)
    {
        close_file(fp);
        return NULL;
    }

    ret->fp    = fp;
    ret->fpos  = 0;
    ret->fsize = get_file_size(fp);
    ret->bsize = buf_size;
    ret->bidx  = 0;
    ret->dsize = 0;

    return ret;
}

char ifstm_next(ifstm_t *fstm)
{
    if(!fstm)
        return FSTM_EOF;

    if(fstm->bidx >= fstm->dsize)
    {
        if(fstm->fpos >= fstm->fsize)
            return FSTM_EOF;

        size_t delta = MIN(fstm->bsize, fstm->fsize - fstm->fpos);

        // 这里没有做关于读取结果的任何检查
        // 因为在这种情况下读取失败是种不可理喻的情况，可能意味着文件系统bug
        // 如果有问题就让它暴露出来好了
        read_file(fstm->fp, fstm->fpos, delta, fstm->buf);

        fstm->dsize = delta;
        fstm->bidx  = 0;
        fstm->fpos += delta;
    }

    return fstm->buf[fstm->bidx++];
}

void ifstm_free(ifstm_t *fstm)
{
    if(!fstm)
        return;
    
    close_file(fstm->fp);
    free(fstm);
}

size_t ifstm_remain_size(ifstm_t *fstm)
{
    return fstm ? (fstm->fsize - fstm->fpos + fstm->dsize - fstm->bidx) : 0;
}
