#ifndef TINYOS_LIB_STRING_H
#define TINYOS_LIB_STRING_H

#include "intdef.h"

size_t strlen(const char *str);

void strcpy(char *dst, const char *src);

int strcmp(const char *lhs, const char *rhs);

void strcat(char *fst, const char *snd);

void uint32_to_str(uint32_t intval, char *buf);

void memset(char *dst, uint8_t val, size_t byte_size);

void memcpy(char *dst, const char *src, size_t byte_size);

uint32_t strhash(const char *str);

#endif /* TINYOS_LIB_STRING_H */
