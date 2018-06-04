#include <shared/atrc.h>

#define ATRC_ELEM_USED (-2)

#define IDX_FIELD(S) (*(uint32_t*)((uint32_t)(S) - 4))

void init_atrc(struct atrc *a, size_t elem_size,
               void *data_zone, size_t data_zone_size)
{
    uint32_t end = data_zone_size / elem_size;

    a->fst_avl_idx = (end ? 0 : ATRC_ELEM_HANDLE_NULL);
    a->data        = data_zone;
    a->total_size  = (int32_t)end;
    
    char *d = (char*)data_zone + 4;
    for(size_t i = 0; i + 1 < end; ++i)
    {
        IDX_FIELD(d) = i + 1;
        d += elem_size;
    }
    IDX_FIELD(d) = ATRC_ELEM_HANDLE_NULL;
}

bool is_atrc_unit_valid(struct atrc *a, size_t elem_size, atrc_elem_handle unit)
{
    return 0 <= unit && unit < a->total_size &&
            get_atrc_unit_idxfield(a, elem_size, unit) == ATRC_ELEM_USED;
}

uint32_t get_atrc_unit_idxfield(struct atrc *a, size_t elem_size, atrc_elem_handle unit)
{
    return IDX_FIELD(get_atrc_unit(a, elem_size, unit));
}

atrc_elem_handle alloc_atrc_unit(struct atrc *a, size_t elem_size)
{
    if(is_atrc_full(a))
        return ATRC_ELEM_HANDLE_NULL;
    
    atrc_elem_handle ret = a->fst_avl_idx;
    void *unit = get_atrc_unit(a, elem_size, ret);
    a->fst_avl_idx = IDX_FIELD(unit);
    IDX_FIELD(unit) = ATRC_ELEM_USED;

    return ret;
}

void free_atrc_unit(struct atrc *a, size_t elem_size, atrc_elem_handle unit)
{
    void *u = get_atrc_unit(a, elem_size, unit);
    IDX_FIELD(u) = a->fst_avl_idx;
    a->fst_avl_idx = unit;
}

atrc_elem_handle atrc_begin(struct atrc *a, size_t elem_size)
{
    for(atrc_elem_handle i = 0; i < a->total_size; ++i)
    {
        if(IDX_FIELD(get_atrc_unit(a, elem_size, i)) == ATRC_ELEM_USED)
            return i;
    }
    return atrc_end();
}

atrc_elem_handle atrc_next(struct atrc *a, size_t elem_size, atrc_elem_handle unit)
{
    for(atrc_elem_handle i = unit + 1; i < a->total_size; ++i)
    {
        if(IDX_FIELD(get_atrc_unit(a, elem_size, i)) == ATRC_ELEM_USED)
            return i;
    }
    return atrc_end();
}
