#ifndef TINY_OS_SHARED_ATRC_H
#define TINY_OS_SHARED_ATRC_H

#include <lib/stdbool.h>
#include <lib/stdint.h>

/*
    atrc是一种提供以下操作的容器：
        1. 创建时指定空间上限，除元素空间外额外空间开销为O(1)
        2. O(1)时间分配一个新元素空间，返回一个该元素的句柄
        3. O(1)时间跟据句柄得到元素空间
        4. O(1)时间跟据句柄释放元素空间
        5. O(n)时间遍历所有元素，其中n是空间上限
    就是数组 + 自由链表封装了一下（

    C语言没有泛型我要死了……_Generic也好意思叫泛型？
*/

typedef int32_t atrc_elem_handle;

#define ATRC_ELEM_HANDLE_NULL (-1)

#define ATRC_ELEM_SIZE(T) \
    ({ struct T_ \
       { \
           T t; int32_t idx_field_padding; \
       }; \
       sizeof(struct T_); })

struct atrc
{
    int32_t fst_avl_idx;
    int32_t total_size;
    void *data;
};

/* elem_size应通过宏ATRC_ELEM_SIZE(elem_type)得到 */
void init_atrc(struct atrc *a, size_t elem_size,
               void *data_zone, size_t data_zone_size);

/* atrc容器是否已经装满 */
#define is_atrc_full(A) (A->fst_avl_idx == ATRC_ELEM_HANDLE_NULL)

/* 取得一个atrc容器中以UNIT为句柄的元素地址 */
#define get_atrc_unit(A, ELEM_SIZE, UNIT) \
    ((void*)((uint32_t)((A)->data) + (ELEM_SIZE) * (UNIT)))

bool is_atrc_unit_valid(struct atrc *a, size_t elem_size, atrc_elem_handle unit);

/* 在一个atrc容器中申请一个元素，返回其句柄。失败时返回ATRC_ELEM_HANDLE_NULL */
atrc_elem_handle alloc_atrc_unit(struct atrc *a, size_t elem_size);

/* 释放一个atrc容器中句柄unit代表的元素 */
void free_atrc_unit(struct atrc *a, size_t elem_size, atrc_elem_handle unit);

/* 以下begin，next，end三剑客和STL迭代器用法相同 */

atrc_elem_handle atrc_begin(struct atrc *a, size_t elem_size);

atrc_elem_handle atrc_next(struct atrc *a, size_t elem_size, atrc_elem_handle unit);

#define atrc_end() ATRC_ELEM_HANDLE_NULL

#endif /* TINY_OS_SHARED_ATRC_H */
