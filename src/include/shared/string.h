#ifndef TINYOS_SHARD_STRING_H
#define TINYOS_SHARD_STRING_H

#include <shared/stdbool.h>
#include <shared/stdint.h>

size_t strlen(const char *str);

void strcpy(char *dst, const char *src);

void strcpy_s(char *dst, const char *src, size_t buf_size);

int strcmp(const char *lhs, const char *rhs);

int strcoll(const char *L, const char *R);

void strcat(char *fst, const char *snd);

#define STRING_NPOS ((uint32_t)0xffffffff)

uint32_t strfind(const char *str, char c, uint32_t beg);

const char *strchr(const char *str, int ch);

void uint32_to_str(uint32_t intval, char *buf);

bool str_to_uint32(const char *str, uint32_t *val);

void memset(void *dst, uint8_t val, size_t byte_size);

void memcpy(void *dst, const void *src, size_t byte_size);

int memcmp(const void *L, const void *R, size_t byte_size);

uint32_t strhash(const char *str);

#endif /* TINYOS_SHARD_STRING_H */
