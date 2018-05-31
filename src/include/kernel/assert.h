#ifndef TINY_OS_FATAL_ERROR_H
#define TINY_OS_FATAL_ERROR_H

#include <shared/utility.h>

#include <config.h>

#define FATAL_ERROR(MSG) _fatal_error_impl("Fatal error in ", \
                                           __FILE__, __FUNCTION__, \
                                           __LINE__, MSG)

#ifdef NO_ASSERT
    #define ASSERT_M(EXPR, MSG) ((void)0)
    #define ASSERT(EXPR) ((void)0)
#else
    #define ASSERT_M(EXPR, MSG) \
        do { \
            if(!(EXPR)) \
                _fatal_error_impl("Assert failed in ", \
                                  __FILE__, __FUNCTION__, \
                                  __LINE__, MSG); \
        } while(0)

    #define ASSERT(EXPR) \
        do { \
            if(!(EXPR)) \
                _fatal_error_impl("Assert failed in ", \
                                  __FILE__, __FUNCTION__, \
                                  __LINE__, "\b\b"); \
        } while(0)
#endif

void _fatal_error_impl(const char *prefix, const char *filename,
                       const char *function, int line, const char *msg);

#endif /* TINY_OS_FATAL_ERROR_H */
