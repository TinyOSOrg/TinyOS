#include <kernel/memory.h>

#include <kernel/filesys/afs/afs.h>
#include <kernel/filesys/afs/blk_mem_buf.h>
#include <kernel/filesys/afs/disk_cache.h>
#include <kernel/filesys/afs/dp_phy.h>
#include <kernel/filesys/afs/file.h>
#include <kernel/filesys/afs/file_type.h>

#include <shared/freelist.h>
#include <shared/string.h>
#include <shared/syscall/filesys.h>

struct dir_head
{
    uint32_t count; // 有效的项数量
    uint32_t zone;  // 项空间容量
};

struct dir_unit
{
    char name[AFS_FILE_NAME_MAX_LENGTH + 1];
    uint32_t entry_index;
};

/* 用来记录空闲dp_head空间的自由链表 */
static freelist_handle dp_head_fl;

static spinlock dp_head_fl_lock;

static struct afs_dp_head *alloc_dp_head()
{
    spinlock_lock(&dp_head_fl_lock);

    if(is_freelist_empty(&dp_head_fl))
    {
        struct afs_dp_head *new_heads = alloc_ker_page(true);
        uint32_t end = 4096 / sizeof(struct afs_dp_head);
        for(size_t i = 0;i < end; ++i)
            add_freelist(&dp_head_fl, &new_heads[i]);
    }

    struct afs_dp_head *ret = fetch_freelist(&dp_head_fl);
    spinlock_unlock(&dp_head_fl_lock);
    return ret;
}

static void free_dp_head(struct afs_dp_head *head)
{
    spinlock_lock(&dp_head_fl_lock);
    add_freelist(&dp_head_fl, head);
    spinlock_unlock(&dp_head_fl_lock);
}

void init_afs()
{
    init_afs_buffer_allocator();
    init_afs_disk_cache();
    init_afs_file();

    init_freelist(&dp_head_fl);
    init_spinlock(&dp_head_fl_lock);
}

void destroy_afs()
{
    afs_release_all_sector_cache();
    afs_release_all_block_cache();
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

struct afs_dp_head *afs_init_dp_handler(uint32_t beg)
{
    struct afs_dp_head *ret = alloc_dp_head();
    afs_init_dp_head(beg, ret);
    return ret;
}

void afs_restore_dp_handler(uint32_t handler)
{
    afs_restore_dp_head((struct afs_dp_head*)handler);
}

void afs_release_dp_handler(uint32_t handler)
{
    struct afs_dp_head *head = (struct afs_dp_head*)handler;
    afs_restore_dp_head(head);
    free_dp_head(head);
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

static const char *path_begin(const char *path, uint32_t *length)
{
    ASSERT_S(path && length);

    if(*path++ != '/' || !*path || *path == '/')
        return NULL;

    unsigned int len = 0;
    while(path[len] && path[len] != '/')
        ++len;

    *length = len;
    return path;
}

static const char *path_next(const char *path, uint32_t *length)
{
    ASSERT_S(path && length);

    while(*path && *path != '/')
        ++path;
    
    if(*path++ != '/' || !*path || *path == '/')
    {
        *length = 0;
        return NULL;
    }

    unsigned int len = 0;
    while(path[len] && path[len] != '/')
        ++len;
        
    *length = len;
    return path;
}

/* 给定一个目录文件，在其中查找给定的文件名 */
static bool find_in_dir(struct afs_dp_head *head,
                        struct afs_file_desc *dir,
                        const char *name, uint32_t name_len,
                        uint32_t *entry_idx,
                        uint32_t *inner_idx,
                        enum afs_file_operation_status *rt)
{
    ASSERT_S(head && dir && name && entry_idx);

    // 取得目录项数量
    struct dir_head H;
    if(!afs_read_binary(head, dir, 0, sizeof(struct dir_head), &H, rt))
        return false;
    
    struct dir_unit unit;
    uint32_t fpos = 0 + sizeof(struct dir_head);
    for(uint32_t i = 0;i != H.count; ++i)
    {
        if(!afs_read_binary(head, dir, fpos,
                sizeof(struct dir_unit), &unit, rt))
            return false;
        
        fpos += sizeof(struct dir_unit);

        if(match_dir_name(unit.name, name, name_len))
        {
            *entry_idx = unit.entry_index;
            if(inner_idx)
                *inner_idx = i;
            return true;
        }
    }

    SET_RT(afs_file_opr_not_found);
    return false;
}

struct afs_file_desc *afs_open_regular_file_for_reading_by_path(
                            struct afs_dp_head *head,
                            const char *path,
                            enum afs_file_operation_status *rt)
{
    // 取出第一截文件名
    uint32_t name_len = 0;
    const char *name_beg = path_begin(path, &name_len);
    if(!name_beg)
    {
        SET_RT(afs_file_opr_not_found);
        return NULL;
    }

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

        if(!name_beg)
        {
            afs_close_file_for_reading(head, dir);
            SET_RT(afs_file_opr_not_found);
            return NULL;
        }

        uint32_t next_entry = 0;
        bool found = find_in_dir(head, dir, name_beg, name_len,
                                 &next_entry, NULL, rt);
    
        afs_close_file_for_reading(head, dir);

        // 没找到入口
        if(!found)
            return NULL; // 若查找失败，rt由find_in_dir设置

        // 取得下一截路径
        name_beg = path_next(name_beg, &name_len);
        
        dir_entry_idx = next_entry;
    }
    ASSERT_S(file != NULL);

    // 此时名字应该已经到末端了，否则出错
    if(name_beg)
    {
        afs_close_file_for_reading(head, file);
        SET_RT(afs_file_opr_not_found);
        return NULL;
    }

    SET_RT(afs_file_opr_success);
    return file;
}

struct afs_file_desc *afs_open_regular_file_for_writing_by_path(
                            struct afs_dp_head *head,
                            const char *path,
                            enum afs_file_operation_status *rt)
{
    struct afs_file_desc *desc =
        afs_open_regular_file_for_reading_by_path(head, path, rt);
    if(!desc)
        return NULL;
    
    if(!afs_convert_reading_to_writing(head, desc))
    {
        afs_close_file_for_reading(head, desc);
        return NULL;
    }
    return desc;
}

struct afs_file_desc *afs_open_dir_file_for_reading_by_path(
                            struct afs_dp_head *head,
                            const char *path,
                            enum afs_file_operation_status *rt)
{
    // 就是单纯的根目录
    if(path[0] == '/' && !path[1])
        return afs_open_file_for_reading(head, head->root_dir_entry, rt);

    // 取出第一截文件名
    uint32_t name_len;
    const char *name_beg = path_begin(path, &name_len);
    if(!name_beg)
    {
        SET_RT(afs_file_opr_not_found);
        return NULL;
    }

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
                                 &next_entry, NULL, rt);
        afs_close_file_for_reading(head, dir);

        if(!found)
            return NULL;

        if(!(name_beg = path_next(name_beg, &name_len)))
        {
            SET_RT(afs_file_opr_not_found);
            return NULL;
        }

        dir_entry_idx = next_entry;
    }

    return NULL;
}

struct afs_file_desc *afs_open_dir_file_for_writing_by_path(
                            struct afs_dp_head *head,
                            const char *path,
                            enum afs_file_operation_status *rt)
{
    struct afs_file_desc *desc =
        afs_open_dir_file_for_reading_by_path(head, path, rt);
    if(!afs_convert_reading_to_writing(head, desc))
    {
        afs_close_file_for_reading(head, desc);
        return NULL;
    }
    return desc;
}

void afs_close_file(struct afs_dp_head *head,
                    struct afs_file_desc *file)
{
    if(afs_is_file_wlocked(file))
        afs_close_file_for_writing(head, file);
    else
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
            sizeof(struct dir_head) + 2 * sizeof(struct dir_unit),
            &trt))
        goto FAILED;

    // 添加.和..文件

    struct dir_head H;
    H.count = 2;
    H.zone  = 2;

    afs_write_binary(head, dir, 0, sizeof(struct dir_head), &H, &trt);
    if(trt != afs_file_opr_success)
        goto FAILED;
    
    uint32_t fpos = sizeof(struct dir_head); struct dir_unit unit;

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

    struct dir_head H;
    if(!afs_read_binary(head, dir, 0, sizeof(struct dir_head), &H, &trt))
    {
        afs_close_file_for_reading(head, dir);
        SET_RT(trt);
        return false;
    }

    if(H.count > 2)
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

/*
    在某个目录文件下添加一个新的空目录文件
    会关闭parent_dir
*/
static void afs_create_dir_in(struct afs_dp_head *head,
                              const char *name_beg, uint32_t name_len,
                              uint32_t dir_entry_idx,
                              struct afs_file_desc *parent_dir,
                              enum afs_file_operation_status *rt)
{
    // 检查目标文件名字长度
    if(name_len > AFS_FILE_NAME_MAX_LENGTH)
    {
        afs_close_file_for_reading(head, parent_dir);
        SET_RT(afs_file_opr_limit_exceeded);
        return;
    }

    // 尝试在父目录中查找当前文件名，避免文件重名
    uint32_t dummy_entry;
    bool found = find_in_dir(head, parent_dir, name_beg, name_len,
                             &dummy_entry, NULL, NULL);
    if(found)
    {
        afs_close_file_for_reading(head, parent_dir);
        SET_RT(afs_file_opr_file_existed);
        return;
    }

    // 将正打开的父目录文件转为可写模式
    if(!afs_convert_reading_to_writing(head, parent_dir))
    {
        afs_close_file_for_reading(head, parent_dir);
        SET_RT(afs_file_opr_reading_lock);
        return;
    }

    // 创建子目录
    enum afs_file_operation_status trt;
    uint32_t entry = afs_create_dir_file_raw(
        head, dir_entry_idx, false, &trt);
    if(trt != afs_file_opr_success)
    {
        afs_close_file_for_writing(head, parent_dir);
        SET_RT(trt);
        return;
    }

    // 更新父目录文件

    struct dir_head H;
    if(!afs_read_binary(head, parent_dir, 0, sizeof(struct dir_head), &H, rt))
    {
        afs_close_file_for_writing(head, parent_dir);
        return;
    }

    if(H.zone == H.count)
    {
        uint32_t new_size = sizeof(struct dir_head) +
                (H.zone + 1) * sizeof(struct dir_unit);

        if(!afs_expand_file(head, parent_dir, new_size, rt))
        {
            afs_close_file_for_writing(head, parent_dir);
            afs_remove_dir_file_raw(head, entry, NULL);
            return;
        }
        ++H.count;
        ++H.zone;
    }
    else
        ++H.count;

    struct dir_unit unit;
    for(uint32_t i = 0;i != name_len; ++i)
        unit.name[i] = name_beg[i];
    unit.name[name_len] = '\0';
    unit.entry_index = entry;
    if(!afs_write_binary(head, parent_dir,
                         sizeof(struct dir_head) + (H.count - 1) * sizeof(struct dir_unit),
                         sizeof(struct dir_unit), &unit, rt) ||
       !afs_write_binary(head, parent_dir, 0, sizeof(struct dir_head),
                         &H, rt))
    {
        afs_close_file_for_writing(head, parent_dir);
        afs_remove_dir_file_raw(head, entry, NULL);
        return;
    }

    afs_extract_file_entry(parent_dir)->byte_size =
        sizeof(struct dir_head) + H.zone * sizeof(struct dir_unit);

    afs_close_file_for_writing(head, parent_dir);
    SET_RT(afs_file_opr_success);
}

/*
    在某个目录文件下添加一个新的常规文件
    会关闭parent_dir
*/
static void afs_create_regular_in(struct afs_dp_head *head,
                                  const char *name_beg, uint32_t name_len,
                                  struct afs_file_desc *parent_dir,
                                  enum afs_file_operation_status *rt)
{
    // 检查目标文件名字长度
    if(name_len > AFS_FILE_NAME_MAX_LENGTH)
    {
        afs_close_file_for_reading(head, parent_dir);
        SET_RT(afs_file_opr_limit_exceeded);
        return;
    }

    // 尝试在父目录中查找当前文件名，避免文件重名
    uint32_t dummy_entry;
    bool found = find_in_dir(head, parent_dir, name_beg, name_len,
                             &dummy_entry, NULL, NULL);
    if(found)
    {
        afs_close_file_for_reading(head, parent_dir);
        SET_RT(afs_file_opr_file_existed);
        return;
    }

    // 将正打开的父目录文件转为可写模式
    if(!afs_convert_reading_to_writing(head, parent_dir))
    {
        afs_close_file_for_reading(head, parent_dir);
        SET_RT(afs_file_opr_reading_lock);
        return;
    }

    // 创建目标文件
    enum afs_file_operation_status trt;
    uint32_t entry = afs_create_empty_file(
        head, AFS_FILE_TYPE_REGULAR, &trt);
    if(trt != afs_file_opr_success)
    {
        afs_close_file_for_writing(head, parent_dir);
        SET_RT(trt);
        return;
    }

    // 更新父目录文件
    struct dir_head H;
    if(!afs_read_binary(head, parent_dir, 0, sizeof(struct dir_head), &H, rt))
    {
        afs_close_file_for_writing(head, parent_dir);
        return;
    }

    if(H.zone == H.count)
    {
        uint32_t new_size = sizeof(struct dir_head) +
                (H.zone + 1) * sizeof(struct dir_unit);

        if(!afs_expand_file(head, parent_dir, new_size, rt))
        {
            afs_close_file_for_writing(head, parent_dir);
            afs_remove_dir_file_raw(head, entry, NULL);
            return;
        }
        ++H.count;
        ++H.zone;
    }
    else
        ++H.count;

    struct dir_unit unit;
    for(uint32_t i = 0;i != name_len; ++i)
        unit.name[i] = name_beg[i];
    unit.name[name_len] = '\0';
    unit.entry_index = entry;
    if(!afs_write_binary(head, parent_dir,
                         sizeof(struct dir_head) + (H.count - 1) * sizeof(struct dir_unit),
                         sizeof(struct dir_unit), &unit, rt) ||
       !afs_write_binary(head, parent_dir, 0, sizeof(struct dir_head),
                         &H, rt))
    {
        afs_close_file_for_writing(head, parent_dir);
        afs_remove_dir_file_raw(head, entry, NULL);
        return;
    }

    afs_extract_file_entry(parent_dir)->byte_size =
        sizeof(struct dir_head) + H.zone * sizeof(struct dir_unit);

    afs_close_file_for_writing(head, parent_dir);
    SET_RT(afs_file_opr_success);
}

void afs_create_dir_file_by_path(struct afs_dp_head *head,
                                 const char *path,
                                 enum afs_file_operation_status *rt)
{
    uint32_t name_len, next_name_len;
    const char *next_name_beg, *name_beg = path_begin(path, &name_len);

    if(!name_beg)
    {
        SET_RT(afs_file_opr_not_found);
        return;
    }

    uint32_t dir_entry_idx = head->root_dir_entry;

    while(true)
    {
        // 先求下一截路径，如果没了，说明当前name就是要创建的目标
        next_name_beg = path_next(name_beg, &next_name_len);

        struct afs_file_desc *parent_dir =
            afs_open_file_for_reading(head, dir_entry_idx, rt);
        if(!parent_dir)
            return;
        
        if(afs_extract_file_entry(parent_dir)->type
                != AFS_FILE_TYPE_DIRECTORY)
        {
            afs_close_file_for_reading(head, parent_dir);
            SET_RT(afs_file_opr_not_found);
            return;
        }
        
        // 当前文件名就是要创建的目标
        if(!next_name_beg)
        {
            afs_create_dir_in(head, name_beg, name_len,
                              dir_entry_idx, parent_dir, rt);
            return;
        }

        uint32_t next_entry;
        bool found = find_in_dir(head, parent_dir,
                        name_beg, name_len, &next_entry, NULL, rt);
        afs_close_file_for_reading(head, parent_dir);

        if(!found)
            return;
        
        dir_entry_idx = next_entry;

        name_beg = next_name_beg;
        name_len = next_name_len;
    }
}

void afs_create_regular_file_by_path(struct afs_dp_head *head,
                                     const char *path,
                                     enum afs_file_operation_status *rt)
{
    uint32_t name_len, next_name_len;
    const char *next_name_beg, *name_beg = path_begin(path, &name_len);

    if(!name_beg)
    {
        SET_RT(afs_file_opr_not_found);
        return;
    }

    uint32_t dir_entry_idx = head->root_dir_entry;

    while(true)
    {
        // 先求下一截路径，如果没了，说明当前name就是要创建的目标
        next_name_beg = path_next(name_beg, &next_name_len);
        struct afs_file_desc *parent_dir =
            afs_open_file_for_reading(head, dir_entry_idx, rt);
        if(!parent_dir)
            return;
        
        if(afs_extract_file_entry(parent_dir)->type
                != AFS_FILE_TYPE_DIRECTORY)
        {
            afs_close_file_for_reading(head, parent_dir);
            SET_RT(afs_file_opr_not_found);
            return;
        }
        
        // 当前文件名就是要创建的目标
        if(!next_name_beg)
        {
            afs_create_regular_in(head, name_beg, name_len, parent_dir, rt);
            return;
        }

        uint32_t next_entry;
        bool found = find_in_dir(head, parent_dir,
                        name_beg, name_len, &next_entry, NULL, rt);
        afs_close_file_for_reading(head, parent_dir);

        if(!found)
            return;
        
        dir_entry_idx = next_entry;

        name_beg = next_name_beg;
        name_len = next_name_len;
    }
}

void afs_remove_file_by_path(struct afs_dp_head *head,
                             const char *path,
                             uint32_t type,
                             enum afs_file_operation_status *rt)
{
    uint32_t next_name_len, name_len;
    const char *next_name_beg = NULL, *name_beg = path_begin(path, &name_len);

    if(!name_beg)
    {
        SET_RT(afs_file_opr_not_found);
        return;
    }

    uint32_t dir_entry_idx = head->root_dir_entry;

    while(true)
    {
        next_name_beg = path_next(name_beg, &next_name_len);

        struct afs_file_desc *parent_dir =
            afs_open_file_for_reading(head, dir_entry_idx, rt);
        if(!parent_dir)
            return;
        
        if(afs_extract_file_entry(parent_dir)->type
            != AFS_FILE_TYPE_DIRECTORY)
        {
            afs_close_file_for_reading(head, parent_dir);
            SET_RT(afs_file_opr_not_found);
            return;
        }

        uint32_t next_entry;
        uint32_t inner_idx;
        bool found = find_in_dir(head, parent_dir,
            name_beg, name_len, &next_entry, &inner_idx, rt);
        
        if(!found)
        {
            afs_close_file_for_reading(head, parent_dir);
            return;
        }

        // 当前文件名就是要删除的文件
        if(!next_name_beg)
        {
            if(!afs_convert_reading_to_writing(head, parent_dir))
            {
                afs_close_file_for_reading(head, parent_dir);
                SET_RT(afs_file_opr_writing_lock);
                return;
            }
            
            // 看看有没有人在用，如果没有，因为父目录被锁了，以后也不会有了
            struct afs_file_desc *dst = afs_open_file_for_writing(
                head, next_entry, NULL);
            if(!dst)
            {
                afs_close_file_for_writing(head, parent_dir);
                SET_RT(afs_file_opr_rm_locked);
                return;
            }
            // 检查文件类型
            if(afs_extract_file_entry(dst)->type != type)
            {
                afs_close_file_for_writing(head, dst);
                afs_close_file_for_writing(head, parent_dir);
                SET_RT(afs_file_opr_rm_wrong_type);
                return;
            }
            afs_close_file_for_writing(head, dst);

            if(type == AFS_FILE_TYPE_REGULAR)
                afs_remove_file(head, next_entry, NULL);
            else if(!afs_remove_dir_file_raw(head, next_entry, rt))
            {
                afs_close_file_for_writing(head, parent_dir);
                return;
            }

            // 把最后一个文件挪到inner_idx处，然后count--
            struct dir_head H;
            afs_read_binary(head, parent_dir, 0, sizeof(struct dir_head), &H, NULL);

            struct dir_unit last_unit;
            afs_read_binary(head, parent_dir,
                sizeof(struct dir_head) + (H.count - 1) * sizeof(struct dir_unit),
                sizeof(struct dir_unit), &last_unit, NULL);
            afs_write_binary(head, parent_dir,
                sizeof(struct dir_head) + inner_idx * sizeof(struct dir_unit),
                sizeof(struct dir_unit), &last_unit, NULL);

            H.count--;
            afs_write_binary(head, parent_dir, 0, sizeof(struct dir_head), &H, NULL);

            afs_close_file_for_writing(head, parent_dir);
            SET_RT(afs_file_opr_success);
            return;
        }

        dir_entry_idx = next_entry;

        name_beg = next_name_beg;
        name_len = next_name_len;
    }
}

uint32_t afs_get_file_byte_size(struct afs_file_desc *file)
{
    return afs_extract_file_entry(file)->byte_size;
}

#include <lib/sys.h>
enum afs_file_operation_status afs_get_dir_unit(struct afs_dp_head *head,
                                                struct afs_file_desc *dir,
                                                uint32_t idx,
                                                struct syscall_filesys_file_info *info)
{
    uint32_t cnt; enum afs_file_operation_status ret;
    if(!afs_read_binary(head, dir, 0, 4, &cnt, &ret))
        return ret;
    
    if(idx >= cnt)
        return afs_file_opr_limit_exceeded;
    
    if(info)
    {
        struct dir_unit unit;
        if(!afs_read_binary(head, dir,
                sizeof(struct dir_head) + idx * sizeof(struct dir_unit),
                sizeof(struct dir_unit), &unit, &ret))
            return ret;
        strcpy_s(info->name, unit.name, FILE_NAME_MAX_LEN + 1);
        
        struct afs_file_entry entry;
        afs_read_file_entry(head, unit.entry_index, &entry);
        info->is_dir = (entry.type == AFS_FILE_TYPE_DIRECTORY);
    }

    return afs_file_opr_success;
}
