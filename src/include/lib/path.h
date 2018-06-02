#ifndef TINY_OS_LIB_PATH_H
#define TINY_OS_LIB_PATH_H

#include <shared/path.h>

/* input包含分区信息，再给个delta，合成并得到dp和分区内路径 */
char *malloc_and_cat_path(const char *input, const char *delta,
                          filesys_dp_handle *out_dp);

#endif /* TINY_OS_LIB_PATH_H */
