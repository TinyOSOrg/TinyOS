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

/* 已经被打开的文件构成的rbtree，从entry_index映射到afs_file_desc */
static struct rb_tree opening_files;
/* rbtree锁 */
static spinlock opening_files_lock;

#define KOF RB_MEM_TO_MEM_OFFSET(struct afs_file_desc, tree_node, entry_idx)

static bool rb_less(const void *L, const void *R)
{
    return *(uint32_t*)L < *(uint32_t*)R;
}

/* 存储空闲file_desc空间的自由链表 */
static freelist_handle file_desc_freelist;
/* file_desc_freelist锁 */
static spinlock file_desc_freelist_lock;

static struct afs_file_desc *alloc_file_desc(void)
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

void init_afs_file(void)
{
    init_spinlock(&opening_files_lock);
    rb_init(&opening_files);
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

    spinlock_lock(&opening_files_lock);
    rb_erase(&opening_files, &file_desc->tree_node, KOF, rb_less);
    spinlock_unlock(&opening_files_lock);

    free_file_desc(file_desc);
}

struct afs_file_desc *afs_open_file_for_reading(
                                struct afs_dp_head *head,
                                uint32_t entry_idx,
                                enum afs_file_operation_status *rt)
{
    spinlock_lock(&opening_files_lock);

    // 先看看是不是已经打开了
    // 如果是读模式打开的，计数++；写模式打开的，则因互斥而返回失败
    
    struct rb_node *rbn = rb_find(&opening_files, KOF, &entry_idx, rb_less);
    if(rbn)
    {
        struct afs_file_desc *desc = GET_STRUCT_FROM_MEMBER(
            struct afs_file_desc, tree_node, rbn);
        if(desc->wlock)
        {
            SET_RT(afs_file_opr_writing_lock);
            spinlock_unlock(&opening_files_lock);
            return NULL;
        }
        else
        {
            ASSERT_S(desc->rlock);
            desc->rlock++;

            spinlock_unlock(&opening_files_lock);
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
    rb_insert(&opening_files, &desc->tree_node, KOF, rb_less);

    spinlock_unlock(&opening_files_lock);

    SET_RT(afs_file_opr_success);
    return desc;
}

/* 打开一个可读写的文件 */
struct afs_file_desc *afs_open_file_for_writing(
                                struct afs_dp_head *head,
                                uint32_t entry_idx,
                                enum afs_file_operation_status *rt)
{
    spinlock_lock(&opening_files_lock);

    // 如果文件已经被打开，则因互斥而失败
    
    struct rb_node *rbn = rb_find(&opening_files, KOF, &entry_idx, rb_less);
    if(rbn)
    {
        struct afs_file_desc *desc = GET_STRUCT_FROM_MEMBER(
            struct afs_file_desc, tree_node, rbn);
        if(desc->wlock)
        {
            SET_RT(afs_file_opr_writing_lock);
            spinlock_unlock(&opening_files_lock);
            return NULL;
        }
        else
        {
            ASSERT_S(desc->rlock);
            spinlock_unlock(&opening_files_lock);
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
    rb_insert(&opening_files, &desc->tree_node, KOF, rb_less);

    spinlock_unlock(&opening_files_lock);

    SET_RT(afs_file_opr_success);
    return desc;
}

void afs_close_file_for_reading(struct afs_dp_head *head,
                                struct afs_file_desc *file)
{
    spinlock_lock(&opening_files_lock);

    ASSERT_S(file->rlock);

    // 若自己是最后一个操作者，则应释放desc
    if(!--file->rlock)
    {
        rb_erase(&opening_files, &file->tree_node, KOF, rb_less);
        free_file_desc(file);
    }

    spinlock_unlock(&opening_files_lock);
}

void afs_close_file_for_writing(struct afs_dp_head *head,
                                struct afs_file_desc *file)
{
    spinlock_lock(&opening_files_lock);

    ASSERT_S(file->wlock && !file->rlock);

    // 写者一定是最后一个操作者，故应写回entry，释放desc
    rb_erase(&opening_files, &file->tree_node, KOF, rb_less);

    spinlock_unlock(&opening_files_lock);

    afs_modify_file_entry(head, file->entry_idx, &file->entry);
    free_file_desc(file);
}

static inline uint32_t pow(uint32_t a, uint32_t b)
{
    uint32_t r = 1;
    while(b > 0)
        r *= a;
    return r;
}

/*
    从block_sec代表的索引树开始的offset_bytes处尝试读取remain_bytes个字节
    返回实际读取的字节数
*/
static uint32_t read_from_index_tree(uint32_t block_sec,
                                     uint32_t offset_bytes,
                                     uint32_t remain_bytes,
                                     void *data)
{
    const struct afs_index_block *block = afs_read_from_block_begin(block_sec);

    // 本次读取的字节数
    uint32_t read_bytes = 0;

    // 孩子为内容块，准备直接读取
    if(block->height == 1)
    {
        uint32_t ch_idx = 0;

        // 按offset跳过一些内容块
        ch_idx = offset_bytes / AFS_BLOCK_BYTE_SIZE;
        offset_bytes %= AFS_BLOCK_BYTE_SIZE;

        while(remain_bytes > 0 && ch_idx < AFS_BLOCK_MAX_CHILD_COUNT)
        {
            uint32_t local_read_bytes = MIN(remain_bytes, AFS_BLOCK_BYTE_SIZE);
            afs_read_from_block(block->child_sec[ch_idx],
                                offset_bytes, local_read_bytes, data);
            
            remain_bytes -= local_read_bytes;
            ++ch_idx;
            offset_bytes = 0;
            read_bytes += local_read_bytes;
            data = (void*)((uint32_t)data + local_read_bytes);
        }

        goto EXIT;
    }

    // 一个孩子对应多少字节的内容
    uint32_t bytes_per_child = pow(AFS_BLOCK_BYTE_SIZE, block->height);

    // 跟据offset跳过一些孩子
    uint32_t ch_idx = offset_bytes / bytes_per_child;
    offset_bytes %= bytes_per_child;

    ASSERT_S(ch_idx < AFS_BLOCK_MAX_CHILD_COUNT);

    while(remain_bytes > 0 && ch_idx < AFS_BLOCK_MAX_CHILD_COUNT)
    {
        uint32_t local_read_bytes = MIN(remain_bytes, bytes_per_child);
        read_from_index_tree(block->child_sec[ch_idx],
                             offset_bytes, local_read_bytes, data);
        
        remain_bytes -= local_read_bytes;
        ++ch_idx;
        offset_bytes = 0;
        read_bytes += local_read_bytes;
        data = (void*)((uint32_t)data + local_read_bytes);
    }

EXIT:

    afs_read_from_block_end(block_sec);
    return read_bytes;
}

bool afs_read_binary(struct afs_dp_head *head,
                     struct afs_file_desc *file,
                     uint32_t fpos, uint32_t bytes,
                     void *data)
{
    // 读取越界
    if(!bytes || fpos + bytes > file->entry.byte_size)
        return false;
    
    // 若文件只包含一个内容块，直接从中拷贝数据即可
    if(!file->entry.index)
    {
        afs_read_from_block(file->entry.sec_beg, fpos, bytes, data);
        return true;
    }

    read_from_index_tree(file->entry.sec_beg, fpos, bytes, data);
    return true;
}
