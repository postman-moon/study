
#define RED         1
#define BLACK       2

typedef int KEY_VALUE;

typedef struct _rbtree_node {
    unsigned char color;
    struct _rbtree_node *left;
    struct _rbtree_node *right;
    struct _rbtree_node *parent;

    KEY_VALUE key;
    void *value;
} rbtree_node;

typedef struct _rbtree {
    struct _rbtree_node *root;
    struct _rbtree_node *nil;
} rbtree;


// 左旋
void rbtree_left_rotate(rbtree *T, rbtree_node *x) {

    rbtree_node *y = x->right;

    x->right = y->left; // 1
    if (y->left != T->nil) {
        y->left->parent = x; // 2
    }

    y->parent = x->parent; // 3
    if (x->parent == T->nil) { // 4
        T->root = y;
    } else if (x == x->parent->left) {
        x->parent->left = y;
    } else {
        x->parent->right = y;
    }

    y->left = x; // 5
    x->parent = y; // 6

}

void rbtree_right_rotate(rbtree *T, rbtree_node *y) {

    rbtree_node *x = y->left;

    y->left = x->right; // 1
    if (x->right != T->nil) {
        x->right->parent = y; // 2
    }

    x->parent = y->parent; // 3
    if (y->parent == T->nil) { // 4
        T->root = x;
    } else if (y == y->parent->right) {
        y->parent->right = x;
    } else {
        y->parent->left = x;
    }

    x->right = y; // 5
    y->parent = x; // 6

}

/*
    三种情况：
    第一种情况：
        父节点是红色，祖父结点是黑色，叔父结点是红色 
        例如：422

    第二种情况：
        父节点是红色，祖父结点是黑色，叔父结点是黑色，自己是左子树
        例如：990

    第三种情况：
        父节点是红色，祖父结点是黑色，叔父结点是黑色，自己是右子树
        例如：1000

*/

void rbtree_insert_fixup(rbtree *T, rbtree_node *z) {

    // 当前节点永远是红色的
    while (z->parent->color == RED) {

        if (z->parent == z->parent->parent->left) {

            rbtree_node *y = z->parent->parent->right;

            if (y->color == RED) {

                z->parent->color = BLACK;
                y->color = BLACK;
                z->parent->parent->color = RED;

                z = z->parent->parent;

            } else {

                if (z == z->parent->right) {
                    z = z->parent;
                    rbtree_left_rotate(T, z);
                }

                z->parent->color = BLACK;
                z->parent->parent->color = RED;
                rbtree_right_rotate(T, z->parent->parent);

            }


        } else {

        }

    }

}

void rbtree_insert(rbtree *T, rbtree_node *z) {

    rbtree_node *y = T->nil;
    rbtree_node *x = T->root;

    while (x != T->nil) {
        y = x;

        if (z->key < x->key) {
            x = x->left;
        } else if (z->key > x->key) {
            x = x->right;
        } else {
            return ;
        }
    }

    z->parent = y;
    if (y = T->nil) {
        T->root = z;
    } else if (z->key < y->key) {
        y->left = z;
    } else {
        y->right = z;
    }

    z->left = T->nil;
    z->right = T->nil;
    z->color = RED;

    rbtree_insert_fixup(T, z);
}

// 后继
rbtree_node *rbtree_successor(rbtree *T, rbtree_node *x) {



}

rbtree_node *rbtree_delete(rbtree *T, rbtree_node *z) {

    rbtree_node *y;
    if ((z->left == T->nil) || (z->right == T->nil)) {
        y = z;
    } else {
        y = rbtree_successor(T, z);
    }

    // if ()

}