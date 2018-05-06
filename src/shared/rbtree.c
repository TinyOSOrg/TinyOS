#include <shared/rbtree.h>
#include <shared/string.h>

#define RB_COLOR_RED   1
#define RB_COLOR_BLACK 0

static inline struct rb_node *rb_get_parent(struct rb_node *node)
{
    return (struct rb_node*)((uint32_t)(node->parent) & (~(uint32_t)0b1));
}

typedef uint32_t rb_color;

static inline rb_color rb_get_color(const struct rb_node *node)
{
    return (rb_color)((uint32_t)(node->parent) & 0b1);
}

static inline void rb_set_parent(struct rb_node *node, struct rb_node *parent)
{
    node->parent = (struct rb_node*)((uint32_t)parent | rb_get_color(node));
}

static inline void rb_set_color(struct rb_node *node, rb_color color)
{
    node->parent = (struct rb_node*)(((uint32_t)node->parent & (~0b1)) | color);
}

#define rb_is_red(node)    ((bool)rb_get_color(node))
#define rb_is_black(node)  (!rb_is_red(node))
#define rb_set_red(node)   rb_set_color(node, RB_COLOR_RED)
#define rb_set_black(node) rb_set_color(node, RB_COLOR_BLACK)

static void rb_left_rotate(struct rb_tree *T, struct rb_node *n)
{
    struct rb_node *y = n->right;
    n->right = y->left;
    if(y->left != &T->nil)
        rb_set_parent(y->left, n);
    rb_set_parent(y, rb_get_parent(n));
    if(rb_get_parent(n) == &T->nil)
        T->root = y;
    else if(n == rb_get_parent(n)->left)
        rb_get_parent(n)->left = y;
    else
        rb_get_parent(n)->right = y;
    y->left = n;
    rb_set_parent(n, y);
}

static void rb_right_rotate(struct rb_tree *T, struct rb_node *n)
{
    struct rb_node *y = n->left;
    n->left = y->right;
    if(y->right != &T->nil)
        rb_set_parent(y->right, n);
    rb_set_parent(y, rb_get_parent(n));
    if(rb_get_parent(n) == &T->nil)
        T->root = y;
    else if(n == rb_get_parent(n)->right)
        rb_get_parent(n)->right = y;
    else
        rb_get_parent(n)->left = y;
    y->right = n;
    rb_set_parent(n, y);
}

/* 把T中的u换成v */
static void rb_transplant(struct rb_tree *T,
                          struct rb_node *u, struct rb_node *v)
{
    struct rb_node *up = rb_get_parent(u);
    if(up == &T->nil)
        T->root = v;
    else if(u == up->left)
        up->left = v;
    else
        up->right = v;
    rb_set_parent(v, up);
}

#define TO_KEY(NODE) \
    ((void*)((char*)(NODE) + key_offset))

void rb_init(struct rb_tree *T)
{
    memset((char*)&T->nil, 0x0, sizeof(T->nil));
    T->root = &T->nil;
}

struct rb_node *rb_find(struct rb_tree *T, int32_t key_offset,
                        const void *key, rb_less_func less)
{
    struct rb_node *p = T->root;
    while(p != &T->nil)
    {
        void *pk = TO_KEY(p);
        if(less(pk, key))
            p = p->right;
        else if(less(key, pk))
            p = p->left;
        else
            return p;
    }
    return NULL;
}

bool rb_insert(struct rb_tree *T, struct rb_node *z,
               int32_t key_offset, rb_less_func less)
{
    struct rb_node *y = &T->nil;
    struct rb_node *x = T->root;

    // 寻找把z插到哪比较合适
    // 循环结束时，z应当插到y的孩子处
    void *zk = TO_KEY(z);
    while(x != &T->nil)
    {
        void *xk = TO_KEY(x);
        y = x;
        if(less(zk, xk))
            x = x->left;
        else if(less(xk, zk))
            x = x->right;
        else
            return false;
    }

    // 将z作为y的孩子
    rb_set_parent(z, y);   
    if(y == &T->nil)
        T->root = z;
    else if(less(zk, TO_KEY(y)))
        y->left = z;
    else
        y->right = z;

    z->left  = &T->nil;
    z->right = &T->nil;
    rb_set_red(z);

    // 修复红黑树性质
    while(rb_is_red(rb_get_parent(z)))
    {
        struct rb_node *zp0  = rb_get_parent(z);
        struct rb_node *zpp0 = rb_get_parent(zp0);
        if(zp0 == zpp0->left)
        {
            y = zpp0->right;
            if(rb_is_red(y))
            {
                rb_set_black(zp0);
                rb_set_black(y);
                rb_set_red(zpp0);
                z = zpp0;
            }
            else if(z == zp0->right)
            {
                z = zp0;
                rb_left_rotate(T, z);
            }

            struct rb_node *zp1  = rb_get_parent(z);
            struct rb_node *zpp1 = rb_get_parent(zp1);
            rb_set_black(zp1);
            rb_set_red(zpp1);
            rb_right_rotate(T, zpp1);
        }
        else
        {
            y = zpp0->left;
            if(rb_is_red(y))
            {
                rb_set_black(zp0);
                rb_set_black(y);
                rb_set_red(zpp0);
                z = zpp0;
            }
            else if(z == zp0->left)
            {
                z = zp0;
                rb_right_rotate(T, z);
            }

            struct rb_node *zp1  = rb_get_parent(z);
            struct rb_node *zpp1 = rb_get_parent(zp1);
            rb_set_black(zp1);
            rb_set_red(zpp1);
            rb_left_rotate(T, zpp1);
        }
    }
    rb_set_black(T->root);

    return true;
}

void rb_erase(struct rb_tree *T, struct rb_node *z,
              int32_t key_offset, rb_less_func less)
{
    struct rb_node *y = z, *x;
    rb_color y_ori_color = rb_get_color(y);

    // 节点摘除
    if(z->left == &T->nil)
    {
        x = z->right;
        rb_transplant(T, z, z->right);
    }
    else if(z->right == &T->nil)
    {
        x = z->left;
        rb_transplant(T, z, z->left);
    }
    else
    {
        y = rb_minimum(&T->nil, z->right);
        y_ori_color = rb_get_color(y);
        x = y->right;

        if(rb_get_parent(y) == z)
            rb_set_parent(x, y);
        else
        {
            rb_transplant(T, y, y->right);
            y->right = z->right;
            rb_set_parent(y->right, y);
        }

        rb_transplant(T, z, y);
        y->left = z->left;
        rb_set_parent(y->left, y);
        rb_set_color(y, rb_get_color(z));
    }

    if(y_ori_color != RB_COLOR_BLACK)
        return;
    
    // 红黑树性质修复
    while(x != T->root && rb_is_black(x))
    {
        if(x == rb_get_parent(x)->left)
        {
            struct rb_node *w = rb_get_parent(x)->right;

            if(rb_is_red(w))
            {
                rb_set_black(w);
                rb_set_red(rb_get_parent(x));
                rb_left_rotate(T, rb_get_parent(x));
                w = rb_get_parent(x)->right;
            }

            if(rb_is_black(w->left) && rb_is_black(w->right))
            {
                rb_set_red(w);
                x = rb_get_parent(x);
            }
            else
            {
                if(rb_is_black(w->right))
                {
                    rb_set_black(w->left);
                    rb_set_red(w);
                    rb_right_rotate(T, w);
                    w = rb_get_parent(x)->right;
                }

                rb_set_color(w, rb_get_color(rb_get_parent(x)));
                rb_set_black(rb_get_parent(x));
                rb_set_black(w->right);
                rb_left_rotate(T, rb_get_parent(x));
                x = T->root;
            }
        }
        else
        {
            struct rb_node *w = rb_get_parent(x)->left;

            if(rb_is_red(w))
            {
                rb_set_black(w);
                rb_set_red(rb_get_parent(x));
                rb_right_rotate(T, rb_get_parent(x));
                w = rb_get_parent(x)->left;
            }

            if(rb_is_black(w->right) && rb_is_black(w->left))
            {
                rb_set_red(w);
                x = rb_get_parent(x);
            }
            else
            {
                if(rb_is_black(w->left))
                {
                    rb_set_black(w->right);
                    rb_set_red(w);
                    rb_left_rotate(T, w);
                    w = rb_get_parent(x)->left;
                }

                rb_set_color(w, rb_get_color(rb_get_parent(x)));
                rb_set_black(rb_get_parent(x));
                rb_set_black(w->left);
                rb_right_rotate(T, rb_get_parent(x));
                x = T->root;
            }
        }
    }

    rb_set_black(x);
}

struct rb_node *rb_minimum(struct rb_node *nil, struct rb_node *node)
{
    struct rb_node *p = node;
    while(p->left != nil)
        p = p->left;
    return p;
}
