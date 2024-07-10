#ifndef __SNPRINTFS_H__
#define __SNPRINTFS_H__

#include <stdarg.h>

/*
 * format supported
 *  integer: %d %i
 *  unsigned integer: %o %u %x %X
 *  string: %s
 *  charactor: %c
 *  pointer: %p
 *  '%': %%
 * modifier support
 *  long integer: l
 *  long long integer: ll
 * */

unsigned long snprintfs(char *str, unsigned long size, const char *format, ...) __attribute__((format(printf, 3, 4)));

unsigned long vsnprintfs(char *str, unsigned long size, const char *format, va_list args);

#endif
