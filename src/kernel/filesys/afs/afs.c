#include <kernel/filesys/afs/afs.h>
#include <kernel/filesys/afs/blk_mem_buf.h>
#include <kernel/filesys/afs/disk_cache.h>
#include <kernel/filesys/afs/dp_phy.h>
#include <kernel/filesys/afs/file.h>

#include <shared/freelist.h>
#include <shared/string.h>

struct dir_unit
{
    char name[AFS_FILE_NAME_MAX_LENGTH + 1];
    uint32_t entry_index;
};

void init_afs()
{
    init_afs_buffer_allocator();
    init_afs_disk_cache();
    init_afs_file();
}

bool afs_reformat_dp(uint32_t beg, uint32_t cnt)
{
    if(!afs_phy_reformat_dp(beg, cnt))
        return false;
    
    struct afs_dp_head head;
    afs_init_dp_head(beg, &head);

    enum afs_file_operation_status rt;
    head.root_dir_entry = afs_create_dir_file_raw(&head, 0, true, &rt);

    afs_restore_dp_head(&head);

    return rt == afs_file_opr_success;
}

#define SET_RT(V) do { if(rt) *rt = (V); } while(0)

static bool match_dir_name(const char *dir_name, const char *path_name,
                                                 uint32_t path_name_len)
{
    uint32_t i = 0;
    while(i < path_name_len && dir_name[i] &&
          dir_name[i] == path_name[i])
        ++i;
    return i == path_name_len && dir_name[i] == '\0';
}

/* 给定一个目录文件，在其中查找给定的文件名 */
static bool find_in_dir(struct afs_dp_head *head,
                        struct afs_file_desc *dir,
                        const char *name, uint32_t name_len,
                        uint32_t *entry_idx,
                        enum afs_file_operation_status *rt)
{
    ASSERT_S(head && dir && name && entry_idx);

    // 取得目录项数量
    uint32_t dir_unit_count;
    if(!afs_read_binary(head, dir, 0, 4, &dir_unit_count, rt))
        return false;
    
    struct dir_unit unit;
    uint32_t fpos = 4;
    for(uint32_t i = 0;i != dir_unit_count; ++i)
    {
        if(!afs_read_binary(head, dir, fpos,
                sizeof(struct dir_unit), &unit, rt))
            return false;
        
        fpos += sizeof(struct dir_unit);

        if(match_dir_name(unit.name, name, name_len))
        {
            *entry_idx = unit.entry_index;
            return true;
        }
    }

    SET_RT(afs_file_opr_not_found);
    return false;
}

struct afs_file_desc *afs_open_regular_file_for_reading(
                            struct afs_dp_head *head,
                            const char *path,
                            enum afs_file_operation_status *rt)
{
    if(*path != '/')
    {
        SET_RT(afs_file_opr_not_found);
        return NULL;
    }

    // 取出第一截文件名
    const char *name_beg = path + 1; uint32_t name_len = 0;
    while(name_beg[name_len] && name_beg[name_len] != '/')
        ++name_len;
    
    uint32_t dir_entry_idx = head->root_dir_entry;

    struct afs_file_desc *file = NULL;
    
    while(true)
    {
        struct afs_file_desc *dir =
            afs_open_file_for_reading(head, dir_entry_idx, rt);
        if(!dir)
            return NULL;
        
        // 遇到一个regular file，跳出循环
        if(afs_extract_file_entry(dir)->type == AFS_FILE_TYPE_REGULAR)
        {
            file = dir;
            break;
        }

        uint32_t next_entry = 0;
        bool found = find_in_dir(head, dir, name_beg, name_len,
                                 &next_entry, rt);
    
        afs_close_file_for_reading(head, dir);

        // 没找到入口
        if(!found)
            return NULL; // 若查找失败，rt由find_in_dir设置

        name_beg += name_len;
        if(name_beg[0] == '\0') // 找得好好的，突然就没了（
        {
            SET_RT(afs_file_opr_not_found);
            return NULL;
        }
        
        // 求出新的name_beg和name_len
        ASSERT_S(name_beg[0] == '/');
        name_beg++;
        name_len = 0;
        while(name_beg[name_len] && name_beg[name_len] != '/')
            ++name_len;
        
        dir_entry_idx = next_entry;
    }
    ASSERT_S(file != NULL);

    // 此时名字应该已经到末端了，否则出错
    if(name_beg[name_len] != '\0')
    {
        afs_close_file_for_reading(head, file);
        SET_RT(afs_file_opr_not_found);
        return NULL;
    }

    SET_RT(afs_file_opr_success);
    return file;
}

struct afs_file_desc *afs_open_regular_file_for_writing(
                            struct afs_dp_head *head,
                            const char *path,
                            enum afs_file_operation_status *rt)
{
    struct afs_file_desc *desc = afs_open_regular_file_for_reading(head, path, rt);
    if(!afs_convert_reading_to_writing(head, desc))
    {
        afs_close_file_for_reading(head, desc);
        return NULL;
    }
    return desc;
}

void afs_close_regular_file(struct afs_dp_head *head,
                            struct afs_file_desc *file)
{
    afs_convert_writing_to_reading(head, file);
    afs_close_file_for_reading(head, file);
}

struct afs_file_desc *afs_open_dir_file_for_reading(
                            struct afs_dp_head *head,
                            const char *path,
                            enum afs_file_operation_status *rt)
{
    if(*path != '/')
    {
        SET_RT(afs_file_opr_not_found);
        return NULL;
    }

    // 取出第一截文件名
    const char *name_beg = path + 1; uint32_t name_len = 0;
    while(name_beg[name_len] && name_beg[name_len] != '/')
        ++name_len;
    
    uint32_t dir_entry_idx = head->root_dir_entry;

    while(true)
    {
        struct afs_file_desc *dir =
            afs_open_file_for_reading(head, dir_entry_idx, rt);
        if(!dir)
            return NULL;

        // 遇到一个regular file，这是个错误……
        if(afs_extract_file_entry(dir)->type == AFS_FILE_TYPE_REGULAR)
        {
            SET_RT(afs_file_opr_not_found);
            afs_close_file_for_reading(head, dir);
            return NULL;
        }

        // 路径正好没了，完美
        if(name_beg[name_len] == '\0')
        {
            SET_RT(afs_file_opr_success);
            return dir;
        }

        uint32_t next_entry = 0;
        bool found = find_in_dir(head, dir, name_beg, name_len,
                                 &next_entry, rt);
        afs_close_file_for_reading(head, dir);

        if(!found)
            return NULL;
        
        name_beg += name_len;
        if(name_beg[0] == '/')
        {
            name_beg++;
            name_len = 0;
            while(name_beg[name_len] && name_beg[name_len] != '/')
                ++name_len;
        }

        dir_entry_idx = next_entry;
    }

    return NULL;
}

struct afs_file_desc *afs_open_dir_file_for_writing(
                            struct afs_dp_head *head,
                            const char *path,
                            enum afs_file_operation_status *rt)
{
    struct afs_file_desc *desc = afs_open_dir_file_for_reading(head, path, rt);
    if(!afs_convert_reading_to_writing(head, desc))
    {
        afs_close_file_for_reading(head, desc);
        return NULL;
    }
    return desc;
}

void afs_close_dir_file(struct afs_dp_head *head,
                        struct afs_file_desc *file)
{
    afs_convert_writing_to_reading(head, file);
    afs_close_file_for_reading(head, file);
}

uint32_t afs_create_dir_file_raw(struct afs_dp_head *head,
                                 uint32_t parent_dir, bool root,
                                 enum afs_file_operation_status *rt)
{
    enum afs_file_operation_status trt;

    uint32_t ret = afs_create_empty_file(head, AFS_FILE_TYPE_DIRECTORY, &trt);
    if(trt != afs_file_opr_success)
    {
        SET_RT(trt);
        return 0;
    }
    
    struct afs_file_desc *dir = afs_open_file_for_writing(head, ret, NULL);
    ASSERT_S(dir != NULL);

    if(!afs_expand_file(head, dir,
            sizeof(uint32_t) + 2 * sizeof(struct dir_unit),
            &trt))
        goto FAILED;

    // 添加.和..文件

    uint32_t dir_size = 2;
    afs_write_binary(head, dir, 0, sizeof(uint32_t), &dir_size, &trt);
    if(trt != afs_file_opr_success)
        goto FAILED;
    
    uint32_t fpos = sizeof(uint32_t); struct dir_unit unit;

    unit.entry_index = ret;
    strcpy(unit.name, ".");
    if(!afs_write_binary(head, dir, fpos,
                         sizeof(struct dir_unit), &unit, &trt))
        goto FAILED;
    
    fpos += sizeof(struct dir_unit);

    unit.entry_index = root ? ret : parent_dir;
    strcpy(unit.name, "..");
    if(!afs_write_binary(head, dir, fpos,
                         sizeof(struct dir_unit), &unit, &trt))
        goto FAILED;
    
    afs_close_file_for_writing(head, dir);
    SET_RT(afs_file_opr_success);
    return ret;

FAILED:

    SET_RT(trt);
    afs_close_file_for_writing(head, dir);
    afs_remove_file(head, ret, NULL);

    return 0;
}

static bool afs_remove_dir_file_raw(struct afs_dp_head *head,
                                    uint32_t entry_idx,
                                    enum afs_file_operation_status *rt)
{
    enum afs_file_operation_status trt;
    struct afs_file_desc *dir = afs_open_file_for_reading(
        head, entry_idx, &trt);
    if(!dir)
    {
        SET_RT(trt);
        return false;
    }

    uint32_t dir_size;
    if(!afs_read_binary(head, dir, 0, 4, &dir_size, &trt))
    {
        afs_close_file_for_reading(head, dir);
        SET_RT(trt);
        return false;
    }

    if(dir_size > 2)
    {
        afs_close_file_for_reading(head, dir);
        SET_RT(afs_file_opr_rm_nonempty);
        return false;
    }

    afs_close_file_for_reading(head, dir);
    afs_remove_file(head, entry_idx, &trt);

    SET_RT(trt);
    return trt == afs_file_opr_success;
}

void afs_create_dir_file(struct afs_dp_head *head,
                         const char *path,
                         enum afs_file_operation_status *rt)
{
    // TODO
    afs_remove_dir_file_raw(head, 0, NULL);
}
