#ifndef TINY_OS_FATAL_ERROR_H
#define TINY_OS_FATAL_ERROR_H

#include <common.h>

#define FATAL_ERROR(MSG) _fatal_error_impl("Fatal error in ", __FILE__, __FUNCTION__, __LINE__, MSG)

#ifdef NO_ASSERT
    #define ASSERT(EXPR, MSG) ((void)0)
    #define ASSERT_S(EXPR) ((void)0)
#else
    #define ASSERT(EXPR, MSG) do { if(!(EXPR)) _fatal_error_impl("Assert failed in ", __FILE__, __FUNCTION__, __LINE__, MSG); } while(0)
    #define ASSERT_S(EXPR) do { if(!(EXPR)) _fatal_error_impl("Assert failed in ", __FILE__, __FUNCTION__, __LINE__, "\b\b"); } while(0)
#endif

void _fatal_error_impl(const char *prefix, const char *filename, const char *function, int line, const char *msg);

#define STATIC_ASSERT(EXPR, MSG) \
    typedef char static_assert_failed__##MSG[2 * !!(EXPR) - 1]

#endif /* TINY_OS_FATAL_ERROR_H */
