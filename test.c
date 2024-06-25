#include "snprintfs.h"

#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <assert.h>

char std_buffer[4096];
char s_buffer[4096];

#define TEST(cap, args...) \
    do { \
        unsigned long result; \
        snprintf(std_buffer, cap, ##args); \
        result = snprintfs(s_buffer, cap, ##args); \
        if(strcmp(std_buffer, s_buffer)) { \
            printf("std: [%s] len=%lu\n", std_buffer, strlen(std_buffer)); \
            printf("sbf: [%s] len=%lu\n", s_buffer, strlen(s_buffer)); \
            assert(0); \
        } \
        if(result != strlen(s_buffer)) { \
            printf("result: %lu\n", result); \
            printf("buffer: [%s]\n", s_buffer); \
            assert(0); \
        } \
        printf("%03lu: [%s]\n", result, s_buffer); \
    } while(0)

#define RTEST(args...) \
    do { \
        for(unsigned i = 1; i < 200u; ++i) { \
            TEST(i, ##args); \
        } \
    } while(0)

int main() {

    RTEST("simple string %%%%%%");
    RTEST("simple integer, %d %i %d %d %d %d", 99999, 99999, 0, -1, 0x7fffffff, 0x80000000);
    RTEST("simple long, %ld %li %ld %ld %ld %ld", 99999L, 99999L, 0L, (long)-1, 0x7fffffffffffffffL, 0x8000000000000000L);
    RTEST("simple unsigned, %u %o %x %X", 99999, 99999, 99999, 99999);
    RTEST("simple unsigned long, %lu %lo %lx %lX", 9999999999L, 9999999999L, 9999999999L, 9999999999L);
    RTEST("simple charactor, %c %c %c %c", 'a', 'b', 'c', 'd');
    RTEST("simple string, %s %s %s %s", "this", "is", "a", "song");
    RTEST("simple pointer, %p %p %p", main, main, main);
    RTEST("mixed, %d %ld %u %o %x %X %lu %lo %lx %lX %c %s %p",
            9999999, 999999999999999L, 9999999, 9999999, 9999999, 9999999,
            999999999999999L, 999999999999999L, 999999999999999L, 999999999999999L, 'X', "song", main);

    return 0;
}
