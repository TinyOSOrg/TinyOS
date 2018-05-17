#include <shared/alloc_arr.h>

#define ALLOC_PTR_ARR_NEXT_NULL (-1)
#define ALLOC_PTR_ARR_NEXT_USED (-2)

void init_alloc_ptr_arr(struct alloc_ptr_arr *arr,
                        void *units_zone, size_t zone_size)
{
    arr->units = (struct alloc_ptr_arr_unit*)units_zone;
    size_t end = zone_size / sizeof(struct alloc_ptr_arr_unit);
    for(size_t i = 0;i + 1 < end; ++i)
    {
        arr->units[i].next_idx = i + 1;
        arr->units[i].ptr      = NULL;
    }
    arr->units[end - 1].next_idx = ALLOC_PTR_ARR_NEXT_NULL;
    arr->units[end - 1].ptr      = NULL;
    arr->fst_avl_unit_idx = 0;
}

bool is_alloc_ptr_arr_full(const struct alloc_ptr_arr *arr)
{
    return arr->fst_avl_unit_idx == ALLOC_PTR_ARR_NEXT_NULL;
}

bool is_alloc_ptr_arr_unit_used(const struct alloc_ptr_arr *arr,
                                size_t idx)
{
    return arr->units[idx].next_idx == ALLOC_PTR_ARR_NEXT_USED;
}

int32_t new_alloc_ptr_arr_unit(struct alloc_ptr_arr *arr, void *ptr)
{
    if(is_alloc_ptr_arr_full(arr))
        return ALLOC_PTR_ARR_NEXT_NULL;

    int32_t idx = arr->fst_avl_unit_idx;
    arr->fst_avl_unit_idx    = arr->units[idx].next_idx;

    arr->units[idx].next_idx = ALLOC_PTR_ARR_NEXT_USED;
    arr->units[idx].ptr      = ptr;

    return idx;
}

void free_alloc_ptr_arr_unit(struct alloc_ptr_arr *arr, size_t idx)
{
    if(!is_alloc_ptr_arr_unit_used(arr, idx))
        return;
    
    arr->units[idx].next_idx = arr->fst_avl_unit_idx;
    arr->units[idx].ptr      = NULL;
    arr->fst_avl_unit_idx    = idx;
}

void *get_alloc_ptr_arr_ptr(struct alloc_ptr_arr *arr, size_t idx)
{
    return arr->units[idx].ptr;
}
