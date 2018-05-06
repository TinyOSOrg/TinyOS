#include <shared/rbtree.h>

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
