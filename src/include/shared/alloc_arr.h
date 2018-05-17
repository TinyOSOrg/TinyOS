#ifndef TINY_OS_SHARED_ALLOC_ARR_H
#define TINY_OS_SHARED_ALLOC_ARR_H

#include <shared/bool.h>
#include <shared/intdef.h>

struct alloc_ptr_arr_unit
{
    int32_t next_idx;
    void *ptr;
};

struct alloc_ptr_arr
{
    int32_t fst_avl_unit_idx;
    struct alloc_ptr_arr_unit *units;
};

#define MAX_ALLOC_PTR_ARR_UNIT_COUNT(TOTAL_BYTE_SIZE) \
    ((TOTAL_BYTE_SIZE) / sizeof(struct alloc_ptr_arr_unit))

void init_alloc_ptr_arr(struct alloc_ptr_arr *arr,
                        void *units_zone, size_t zone_size);

bool is_alloc_ptr_arr_full(const struct alloc_ptr_arr *arr);

bool is_alloc_ptr_arr_unit_used(const struct alloc_ptr_arr *arr, size_t idx);

/* 申请一个新的arr_unit，返回其下标；失败时返回负数 */
int32_t new_alloc_ptr_arr_unit(struct alloc_ptr_arr *arr, void *ptr);

void free_alloc_ptr_arr_unit(struct alloc_ptr_arr *arr, size_t idx);

void *get_alloc_ptr_arr_ptr(struct alloc_ptr_arr *arr, size_t idx);

#endif /* TINY_OS_SHARED_ALLOC_ARR_H */
