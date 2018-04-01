#ifndef TINYOS_STRING_H
#define TINYOS_STRING_H

#include <lib/intdef.h>

size_t _strlen(const char *str);
void _strcpy(char *dst, const char *src);
int _strcmp(const char *lhs, const char *rhs);
void _strcat(char *fst, const char *snd);

void _uint32_to_str(uint32_t intval, char *buf);

#endif //TINYOS_STRING_H
