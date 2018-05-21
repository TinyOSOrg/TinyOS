#ifndef TINY_OS_LIB_FILESYS_H
#define TINY_OS_LIB_FILESYS_H

#include <shared/bool.h>
#include <shared/filesys/filesys.h>
#include <shared/syscall/filesys.h>

enum filesys_opr_result open_file(filesys_dp_handle dp,
                                  const char *path, bool writing,
                                  usr_file_handle *result);

enum filesys_opr_result close_file(usr_file_handle file);

enum filesys_opr_result make_file(filesys_dp_handle dp,
                                  const char *path);

enum filesys_opr_result remove_file(filesys_dp_handle dp,
                                    const char *path);

enum filesys_opr_result make_directory(filesys_dp_handle dp,
                                       const char *path);

enum filesys_opr_result remove_directory(filesys_dp_handle dp,
                                         const char *path);

uint32_t get_file_size(usr_file_handle file);

enum filesys_opr_result write_file(usr_file_handle file,
                                  uint32_t fpos, uint32_t byte_size,
                                  const void *data);

enum filesys_opr_result read_file(usr_file_handle file,
                                  uint32_t fpos, uint32_t byte_size,
                                  void *data);

#endif /* TINY_OS_LIB_FILESYS_H */
