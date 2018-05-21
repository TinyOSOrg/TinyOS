#include <kernel/interrupt.h>
#include <kernel/memory.h>
#include <kernel/process/semaphore.h>
#include <kernel/process/spinlock.h>

#include <kernel/filesys/afs/dp_phy.h>
#include <kernel/filesys/afs/disk_cache.h>
#include <kernel/filesys/afs/file.h>

#include <shared/freelist.h>
#include <shared/ptrlist.h>
#include <shared/utility.h>

struct afs_file_desc
{
    struct rb_node tree_node;
    uint32_t entry_idx;

    // 文件entry在内存中也存一份，方便快速访问
    // 若文件被关闭时最后的操作者是写者，才更新磁盘上的entry
    struct afs_file_entry entry;

    unsigned int rlock : 15; // 读取锁
    unsigned int wlock : 1;  // 写入锁
};

#define KOF MEM_TO_MEM_OFFSET(struct afs_file_desc, tree_node, entry_idx)

static bool rb_less(const void *L, const void *R)
{
    return *(uint32_t*)L < *(uint32_t*)R;
}

/* 存储空闲file_desc空间的自由链表 */
static freelist_handle file_desc_freelist;
/* file_desc_freelist锁 */
static spinlock file_desc_freelist_lock;

static struct afs_file_desc *alloc_file_desc()
{
    spinlock_lock(&file_desc_freelist_lock);

    if(is_freelist_empty(&file_desc_freelist))
    {
        struct afs_file_desc *descs = (struct afs_file_desc*)
            alloc_ker_page(true);
        uint32_t end = 4096 / sizeof(struct afs_file_desc);
        for(size_t i = 0;i != end; ++i)
            add_freelist(&file_desc_freelist, &descs[i]);
    }

    struct afs_file_desc *ret = (struct afs_file_desc*)
        fetch_freelist(&file_desc_freelist);
    spinlock_unlock(&file_desc_freelist_lock);
    return ret;
}

static void free_file_desc(struct afs_file_desc *desc)
{
    spinlock_lock(&file_desc_freelist_lock);
    add_freelist(&file_desc_freelist, desc);
    spinlock_unlock(&file_desc_freelist_lock);
}

#define AFS_BLOCK_MAX_CHILD_COUNT \
    ((AFS_BLOCK_BYTE_SIZE - 8) / sizeof(uint32_t))

/* 索引块结构 */
struct afs_index_block
{
    // 在索引树中的高度。叶节点（内容块）高度为0,以此类推
    uint32_t height;
    // 有多少个有效孩子节点
    uint32_t child_count;
    // 孩子们的首扇区号
    uint32_t child_sec[AFS_BLOCK_MAX_CHILD_COUNT];
};

STATIC_ASSERT(sizeof(struct afs_index_block) == AFS_BLOCK_BYTE_SIZE,
              invalid_size_of_afs_index_block);

#define SET_RT(V) do { if(rt) *rt = (V); } while(0)

static uint32_t full_index_tree_bytes[12];

void init_afs_file()
{
    init_spinlock(&file_desc_freelist_lock);
    init_freelist(&file_desc_freelist);

    full_index_tree_bytes[0] = AFS_BLOCK_BYTE_SIZE;
    for(size_t i = 1;i != 12; ++i)
        full_index_tree_bytes[i] = full_index_tree_bytes[i - 1] * AFS_BLOCK_BYTE_SIZE;
}

uint32_t afs_create_empty_file(struct afs_dp_head *head,
                               uint32_t type,
                               enum afs_file_operation_status *rt)
{
    // 分配一个空block作为内容块
    uint32_t content_block = afs_alloc_disk_block(head);
    if(!content_block)
    {
        SET_RT(afs_file_opr_no_empty_block);
        return 0;
    }

    // 填充entry
    struct afs_file_entry entry =
    {
        .sec_beg = content_block,
        .byte_size = 0,
        .index = 0,
        .type = type
    };

    uint32_t ret;
    if(!afs_alloc_file_entry(head, &entry, &ret))
    {
        afs_free_disk_block(head, content_block);
        SET_RT(afs_file_opr_no_empty_entry);
        return 0;
    }

    SET_RT(afs_file_opr_success);
    return ret;
}

/*
    递归地释放磁盘上的索引树中的块
    root_sec为索引数根部所在块，该块也会被释放

    涉及到的块的互斥由afs_remove_file保证，所以随便操作，不用担心别的线程打扰
*/
static void release_index_tree(struct afs_dp_head *head, uint32_t root_sec)
{
    const struct afs_index_block *blk = (const struct afs_index_block*)
        afs_read_from_block_begin(root_sec);

    ASSERT_S(blk->height > 0);

    // 下一层就是叶节点，则直接释放块
    if(blk->height == 1)
    {
        for(uint32_t i = 0;i < blk->child_count; ++i)
            afs_free_disk_block(head, blk->child_sec[i]);
    }
    else
    {
        for(size_t i = 0;i < blk->child_count; ++i)
            release_index_tree(head, blk->child_sec[i]);
    }

    afs_read_from_block_end(root_sec);

    // 释放自身所在块
    // 释放之后该块的缓存可能还留在disk_cache中，不过这并没有关系
    afs_free_disk_block(head, root_sec);
}

/*
    1. 给文件加写入锁，免得删除的时候别的线程干扰
    2. 释放磁盘上entry中的块
    3. 释放磁盘上的entry
    4. 不走close_file线路，直接把desc从rbtree中摘除并释放

    注意顺序是不能调换的
    如果先走4后走3，那么把desc从rbtree中摘除后别的线程可能成功“打开”这个文件导致GG
*/
void afs_remove_file(struct afs_dp_head *head, uint32_t entry_idx,
                     enum afs_file_operation_status *rt)
{
    // 以可读写模式打开这个文件，失败说明有人在用，guna
    enum afs_file_operation_status opening_rt;
    struct afs_file_desc *file_desc = afs_open_file_for_writing(
        head, entry_idx, &opening_rt);
    if(!file_desc)
    {
        SET_RT(opening_rt);
        return;
    }

    struct afs_file_entry *entry = &file_desc->entry;

    // 如果只有一个内容块，直接释放即可
    if(entry->index == 0)
    {
        afs_free_disk_block(head, entry->sec_beg);
        SET_RT(afs_file_opr_success);
        goto RELEASE_ENTRY;
    }

    // 对索引树结构，递归地释放所有索引块
    ASSERT_S(entry->index);
    release_index_tree(head, entry->sec_beg);
    SET_RT(afs_file_opr_success);

RELEASE_ENTRY:

    afs_free_file_entry(head, entry_idx);

    spinlock_lock(&head->opening_files_lock);
    rb_erase(&head->opening_files, &file_desc->tree_node, KOF, rb_less);
    spinlock_unlock(&head->opening_files_lock);

    free_file_desc(file_desc);
}

struct afs_file_desc *afs_open_file_for_reading(
                                struct afs_dp_head *head,
                                uint32_t entry_idx,
                                enum afs_file_operation_status *rt)
{
    spinlock_lock(&head->opening_files_lock);

    // 先看看是不是已经打开了
    // 如果是读模式打开的，计数++；写模式打开的，则因互斥而返回失败
    
    struct rb_node *rbn = rb_find(&head->opening_files, KOF, &entry_idx, rb_less);
    if(rbn)
    {
        struct afs_file_desc *desc = GET_STRUCT_FROM_MEMBER(
            struct afs_file_desc, tree_node, rbn);
        if(desc->wlock)
        {
            SET_RT(afs_file_opr_writing_lock);
            spinlock_unlock(&head->opening_files_lock);
            return NULL;
        }
        else
        {
            ASSERT_S(desc->rlock);
            desc->rlock++;

            spinlock_unlock(&head->opening_files_lock);
            SET_RT(afs_file_opr_success);
            return desc;
        }
    }

    // 没打开的话，新创建一个desc

    struct afs_file_desc *desc = alloc_file_desc();

    desc->entry_idx = entry_idx;
    desc->rlock     = 1;
    desc->wlock     = 0;
    afs_read_file_entry(head, entry_idx, &desc->entry);
    rb_insert(&head->opening_files, &desc->tree_node, KOF, rb_less);

    spinlock_unlock(&head->opening_files_lock);

    SET_RT(afs_file_opr_success);
    return desc;
}

/* 打开一个可读写的文件 */
struct afs_file_desc *afs_open_file_for_writing(
                                struct afs_dp_head *head,
                                uint32_t entry_idx,
                                enum afs_file_operation_status *rt)
{
    spinlock_lock(&head->opening_files_lock);

    // 如果文件已经被打开，则因互斥而失败
    
    struct rb_node *rbn = rb_find(&head->opening_files, KOF, &entry_idx, rb_less);
    if(rbn)
    {
        struct afs_file_desc *desc = GET_STRUCT_FROM_MEMBER(
            struct afs_file_desc, tree_node, rbn);
        if(desc->wlock)
        {
            SET_RT(afs_file_opr_writing_lock);
            spinlock_unlock(&head->opening_files_lock);
            return NULL;
        }
        else
        {
            ASSERT_S(desc->rlock);
            spinlock_unlock(&head->opening_files_lock);
            SET_RT(afs_file_opr_reading_lock);
            return NULL;
        }
    }

    // 没打开的话，新创建一个desc

    struct afs_file_desc *desc = alloc_file_desc();

    desc->entry_idx = entry_idx;
    desc->rlock     = 0;
    desc->wlock     = 1;
    afs_read_file_entry(head, entry_idx, &desc->entry);
    rb_insert(&head->opening_files, &desc->tree_node, KOF, rb_less);

    spinlock_unlock(&head->opening_files_lock);

    SET_RT(afs_file_opr_success);
    return desc;
}

bool afs_convert_reading_to_writing(struct afs_dp_head *head,
                                    struct afs_file_desc *desc)
{
    ASSERT_S(desc != NULL);
    spinlock_lock(&head->opening_files_lock);

    bool ret = false;
    if(desc->rlock == 1)
    {
        desc->rlock = 0;
        desc->wlock = 1;
        ret = true;
    }

    spinlock_unlock(&head->opening_files_lock);
    return ret;
}

bool afs_convert_writing_to_reading(struct afs_dp_head *head,
                                    struct afs_file_desc *desc)
{
    ASSERT_S(desc != NULL);
    spinlock_lock(&head->opening_files_lock);

    bool ret = false;
    if(desc->wlock == 1)
    {
        desc->wlock = 0;
        desc->rlock = 1;
        ret = true;
    }

    spinlock_unlock(&head->opening_files_lock);
    return ret;
}

void afs_close_file_for_reading(struct afs_dp_head *head,
                                struct afs_file_desc *file)
{
    spinlock_lock(&head->opening_files_lock);

    ASSERT_S(file->rlock);

    // 若自己是最后一个操作者，则应释放desc
    if(!--file->rlock)
    {
        rb_erase(&head->opening_files, &file->tree_node, KOF, rb_less);
        free_file_desc(file);
    }

    spinlock_unlock(&head->opening_files_lock);
}

void afs_close_file_for_writing(struct afs_dp_head *head,
                                struct afs_file_desc *file)
{
    spinlock_lock(&head->opening_files_lock);

    ASSERT_S(file->wlock && !file->rlock);

    // 写者一定是最后一个操作者，故应写回entry，释放desc
    rb_erase(&head->opening_files, &file->tree_node, KOF, rb_less);

    spinlock_unlock(&head->opening_files_lock);

    afs_modify_file_entry(head, file->entry_idx, &file->entry);
    free_file_desc(file);
}

/*
    从block_sec代表的索引树边缘的offset_bytes处读取remain_bytes个字节
*/
static void read_from_index_tree(uint32_t block_sec,
                                 uint32_t offset_bytes,
                                 uint32_t remain_bytes,
                                 void *data)
{
    const struct afs_index_block *block = afs_read_from_block_begin(block_sec);

    // 如何读孩子节点
    void (*reader)(uint32_t, uint32_t, uint32_t, void*) = block->height == 1 ?
        afs_read_from_block : read_from_index_tree;

    // 一个孩子对应多少字节的内容
    uint32_t bytes_per_child = full_index_tree_bytes[block->height - 1];;

    // 跟据offset跳过一些孩子
    uint32_t ch_idx = offset_bytes / bytes_per_child;
    offset_bytes %= bytes_per_child;

    ASSERT_S(ch_idx < AFS_BLOCK_MAX_CHILD_COUNT);

    while(remain_bytes > 0 && ch_idx < AFS_BLOCK_MAX_CHILD_COUNT)
    {
        uint32_t local_read_bytes = MIN(remain_bytes, bytes_per_child);
        reader(block->child_sec[ch_idx], offset_bytes, local_read_bytes, data);
        
        remain_bytes -= local_read_bytes;
        ++ch_idx;
        offset_bytes = 0;
        data = (void*)((uint32_t)data + local_read_bytes);
    }

    afs_read_from_block_end(block_sec);
}

bool afs_read_binary(struct afs_dp_head *head,
                     struct afs_file_desc *file,
                     uint32_t fpos, uint32_t bytes,
                     void *data,
                     enum afs_file_operation_status *rt)
{
    // 读取越界
    if(!bytes || fpos + bytes > file->entry.byte_size)
    {
        SET_RT(afs_file_opr_limit_exceeded);
        return false;
    }
    
    // 若文件只包含一个内容块，直接从中拷贝数据即可
    if(!file->entry.index)
    {
        afs_read_from_block(file->entry.sec_beg, fpos, bytes, data);
        SET_RT(afs_file_opr_success);
        return true;
    }

    read_from_index_tree(file->entry.sec_beg, fpos, bytes, data);
    SET_RT(afs_file_opr_success);
    return true;
}

static void write_to_index_tree(uint32_t block_sec,
                                uint32_t offset_bytes,
                                uint32_t remain_bytes,
                                const void *data)
{
    const struct afs_index_block *block = afs_read_from_block_begin(block_sec);

    // 如何写入孩子节点
    void (*writer)(uint32_t, uint32_t, uint32_t, const void*) =
        block->height == 1 ? afs_write_to_block : write_to_index_tree;

    // 一个孩子对应多少字节的内容
    uint32_t bytes_per_child = full_index_tree_bytes[block->height - 1];;

    // 跟据offset跳过一些孩子
    uint32_t ch_idx = offset_bytes / bytes_per_child;
    offset_bytes %= bytes_per_child;

    ASSERT_S(ch_idx < AFS_BLOCK_MAX_CHILD_COUNT);

    while(remain_bytes > 0 && ch_idx < AFS_BLOCK_MAX_CHILD_COUNT)
    {
        uint32_t local_write_bytes = MIN(remain_bytes, bytes_per_child);

        writer(block->child_sec[ch_idx], offset_bytes, local_write_bytes, data);
        
        remain_bytes -= local_write_bytes;
        ++ch_idx;
        offset_bytes = 0;
        data = (void*)((uint32_t)data + local_write_bytes);
    }

    afs_read_from_block_end(block_sec);
}

bool afs_write_binary(struct afs_dp_head *head,
                      struct afs_file_desc *file,
                      uint32_t fpos, uint32_t bytes,
                      const void *data,
                      enum afs_file_operation_status *rt)
{
    // 文件为只读
    if(!file->wlock)
    {
        SET_RT(afs_file_opr_read_only);
        return false;
    }

    // 写入越界
    if(!bytes || fpos + bytes > file->entry.byte_size)
    {
        SET_RT(afs_file_opr_limit_exceeded);
        return false;
    }

    // 若只包含一个内容块，可直接从中拷贝
    if(!file->entry.index)
    {
        afs_write_to_block(file->entry.sec_beg, fpos, bytes, data);
        SET_RT(afs_file_opr_success);
        return true;
    }
    
    write_to_index_tree(file->entry.sec_beg, fpos, bytes, data);
    SET_RT(afs_file_opr_success);
    return true;
}

/*
    扩充一棵索引树的容量
    blk_sec为子树根部节点扇区号
    若new_root非空，则当整个索引树容量不足时，树高会增加且该变量会被置为新的根
                   否则，会将剩余的扩充任务还给caller
    used_bytes为当前子树中已经有的字节数
    exp_bytes为待扩充的容量
    exp_rt为实际扩充了的容量

    当扩充失败（比如磁盘空间不足）时返回false，此时rt会指出错误原因
*/
static bool expand_index_tree(struct afs_dp_head *head,
                              uint32_t blk_sec,
                              uint32_t *new_root,
                              uint32_t used_bytes,
                              uint32_t exp_bytes,
                              uint32_t *exp_rt,
                              enum afs_file_operation_status *rt)
{
    struct afs_index_block *block = afs_write_to_block_begin(blk_sec);
    uint32_t exp = 0, ch_idx = block->child_count - 1;

    uint32_t old_height = block->height;

    // 如果孩子是content block，遍历之
    if(old_height == 1)
    {    
        // 最后一个孩子节点已经用了多少字节
        uint32_t content_used = used_bytes -
            (block->child_count - 1) * AFS_BLOCK_BYTE_SIZE;
            
        while(exp < exp_bytes &&
              ch_idx < AFS_BLOCK_MAX_CHILD_COUNT)
        {
            if(ch_idx >= block->child_count)
            {
                block->child_sec[ch_idx] = afs_alloc_disk_block(head);
                block->child_count++;
            }

            uint32_t dexp = MIN(AFS_BLOCK_BYTE_SIZE - content_used,
                                exp_bytes - exp);
                                
            exp += dexp;
            content_used = 0;
            ++ch_idx;
        }
    }
    else
    {
        // 最后一个孩子节点已经用了多少字节
        uint32_t child_used = used_bytes -
            (block->child_count - 1) * full_index_tree_bytes[old_height - 1];

        while(exp < exp_bytes &&
              ch_idx < AFS_BLOCK_MAX_CHILD_COUNT)
        {
            if(ch_idx >= block->child_count)
            {
                // 给孩子申请一个新索引节点
                block->child_sec[ch_idx] = afs_alloc_disk_block(head);
                struct afs_index_block *ch_blk =
                    afs_write_to_block_begin(block->child_sec[ch_idx]);

                ch_blk->height = old_height - 1;
                ch_blk->child_count = 0;

                afs_write_to_sector_end(block->child_sec[ch_idx]);
                block->child_count++;
            }

            uint32_t dexp = 0;
            if(!expand_index_tree(head, block->child_sec[ch_idx], NULL,
                                  child_used, exp_bytes - exp, &dexp, rt))
            {
                afs_write_to_block_end(blk_sec);
                exp += dexp;
                *exp_rt = exp;
                return false;
            }

            exp += dexp;
            child_used = 0;
            ++ch_idx;
        }
    }

    afs_write_to_block_end(blk_sec);

    if(exp >= exp_bytes || !new_root)
    {
        *exp_rt = exp;
        SET_RT(afs_file_opr_success);
        return true;
    }

    // 这棵树已经满了，且是root，此时应分配一个新根

    uint32_t new_root_sec = afs_alloc_disk_block(head);
    struct afs_index_block *new_block = afs_write_to_block_begin(new_root_sec);

    new_block->height = old_height + 1;
    new_block->child_count = 1;
    new_block->child_sec[0] = blk_sec;

    afs_write_to_block_end(new_root_sec);
    
    *new_root = new_root_sec;
    uint32_t dexp = 0;
    bool ret = expand_index_tree(head, new_root_sec, new_root,
                                 full_index_tree_bytes[old_height],
                                 exp_bytes - exp, &dexp, rt);
    *exp_rt = exp + dexp;

    return ret;
}

bool afs_expand_file(struct afs_dp_head *head,
                     struct afs_file_desc *file,
                     uint32_t new_size,
                     enum afs_file_operation_status *rt)
{
    // 文件必须是可写的
    if(!file->wlock)
    {
        SET_RT(afs_file_opr_read_only);
        return false;
    }

    // 只能扩大不能减小
    if(new_size <= file->entry.byte_size)
    {
        SET_RT(afs_file_opr_invalid_new_size);
        return false;
    }
    
    // 如果new_size也只需要一个内容块，那么直接改entry中的大小即可
    if(new_size <= AFS_BLOCK_BYTE_SIZE)
    {
        file->entry.byte_size = new_size;
        SET_RT(afs_file_opr_success);
        return true;
    }

    // 如果new_size超过一个内容块但目前只有一个块，将其扩充为一棵索引树
    if(!file->entry.index)
    {
        uint32_t new_root = afs_alloc_disk_block(head);
        struct afs_index_block *root =
            afs_write_to_block_begin(new_root);
        
        root->height = 1;
        root->child_count = 1;
        root->child_sec[0] = file->entry.sec_beg;

        afs_write_to_block_end(new_root);

        file->entry.sec_beg   = new_root;
        file->entry.index     = 1;
        file->entry.byte_size = AFS_BLOCK_BYTE_SIZE;
    }

    uint32_t dexp;
    expand_index_tree(head, file->entry.sec_beg, &file->entry.sec_beg,
                      file->entry.byte_size, new_size - file->entry.byte_size,
                      &dexp, rt);
    file->entry.byte_size += dexp;

    return true;
}

bool afs_is_file_open(struct afs_dp_head *head, uint32_t entry)
{
    spinlock_lock(&head->opening_files_lock);

    struct rb_node *rbn = rb_find(&head->opening_files, KOF, &entry, rb_less);
    bool ret = rbn != NULL;

    spinlock_unlock(&head->opening_files_lock);
    return ret;
}

struct afs_file_entry *afs_extract_file_entry(struct afs_file_desc *desc)
{
    return &desc->entry;
}

bool afs_is_file_wlocked(struct afs_file_desc *desc)
{
    return desc->wlock != 0;
}
