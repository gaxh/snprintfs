#ifndef __SNPRINTFS_H__
#define __SNPRINTFS_H__

#include <stdarg.h>

/*
 * format supported
 *  integer: %d %i
 *  long integer: %ld %li
 *  unsigned integer: %o %u %x %X
 *  unsigned long integer: %lo %lu %lx %lX
 *  string: %s
 *  charactor: %c
 *  pointer: %p
 *  '%': %%
 * */

unsigned long snprintfs(char *str, unsigned long size, const char *format, ...) __attribute__((format(printf, 3, 4)));

unsigned long vsnprintfs(char *str, unsigned long size, const char *format, va_list args);

#endif
