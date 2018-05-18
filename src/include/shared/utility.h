#ifndef TINY_OS_SHARED_UTILITY_H
#define TINY_OS_SHARED_UTILITY_H

#define MAX(a, b) ((a) > (b) ? (a) : (b))

#define MIN(a, b) ((a) < (b) ? (a) : (b))

/*
    给定一个结构名和一个成员名，取得 -(成员在结构体内的偏移量)
    在成员指针上加上这一偏移量，就能得到结构体指针
*/
#define MEM_TO_STRUCT_OFFSET(STRUCT, MEM) \
    (-(int32_t)(&((STRUCT*)0->MEM)))

/*
    给定一个结构体名和其中的两个成员名，取得 成员2偏移量 - 成员1偏移量
    在成员1指针上加上该偏移，就能得到成员2指针
*/
#define MEM_TO_MEM_OFFSET(STRUCT, MEM1, MEM2) \
    ((int32_t)(&(((STRUCT*)0)->MEM2)) - \
     (int32_t)(&(((STRUCT*)0)->MEM1)))

/* 跟据成员地址取得包含它的结构体地址 */
#define GET_STRUCT_FROM_MEMBER(STU, MEM, p_mem) \
    ((STU*)((char*)(p_mem) - (char*)(&((STU*)0)->MEM)))

#define ARRAY_SIZE(A) (sizeof(A) / sizeof(A[0]))

#endif /* TINY_OS_SHARED_UTILITY_H */
