#ifndef TINY_OS_PAGE_DESC_H
#define TINY_OS_PAGE_DESC_H

#include <shared/intdef.h>

typedef uint32_t page_desc_attrib;

#define PAGE_PRESENT_TRUE  ((page_desc_attrib)1)
#define PAGE_PRESENT_FALSE ((page_desc_attrib)0)

#define PAGE_RW_READ_ONLY  ((page_desc_attrib)(0 << 1))
#define PAGE_RW_READ_WRITE ((page_desc_attrib)(1 << 1))

#define PAGE_USER_USER  ((page_desc_attrib)(1 << 2))
#define PAGE_USER_SUPER ((page_desc_attrib)(0 << 2))

#endif /* TINY_OS_PAGE_DESC_H */
