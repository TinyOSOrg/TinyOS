#ifndef TINY_OS_LIB_ASSERT_H
#define TINY_OS_LIB_ASSERT_H

/*
    emm，这个宏在fail的时候只能挡住它所在的线程
    要让assert能多线程运作，需要OS支持，我懒得加了……
*/
#ifndef _LIB_ASSERT_OFF
    #define assert(EXPL) \
        do { \
            extern void _lib_assert_impl(const char *, \
                                         const char *, \
                                         int); \
            if(!(EXPL)) \
                _lib_assert_impl(__FILE__, __FUNCTION__, __LINE__); \
        } while(0)
#else
    #define assert(EXPL) do { } while(0)
#endif

#endif /* TINY_OS_LIB_ASSERT_H */
