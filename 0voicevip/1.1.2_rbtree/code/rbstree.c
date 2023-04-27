#include <stdio.h>

#define RED     0
#define BLACK   1

typedef int KEY_TYPE;

typedef struct _rbtree_node {

    // 红黑树的性质 start
    unsigned char color;
    struct _rbtree_node *parent;
    struct _rbtree_node *left;
    struct _rbtree_node *right;
    // 红黑树的性质 end

    KEY_TYPE key;
} rbtree_node;

typedef struct _rbtree {
    // 根节点
    rbtree_node *root;
    /* 
        NULL，也叫哨兵，作用：纯粹是让大家更好的去判断，以及所有叶子节点都指向它
        当作判断空节点和叶子节点的标准

        如果直接使用 NULL 的话，这个过程中他的叶子节点不是黑色的
     */
    rbtree_node *nil; 
} rbtree;

// rotate
// void rbtree_left_rotate(rbtree *T, rbtree_node *x) {

//     if (x == T->nil) return ;

//     // 第一个方向
//     // 1. x的右节点为y
//     rbtree_node *y = x->right;

//     // 2. 将y的左子节点设为x的右子节点
//     x->right = y->left;
//     // 3. 如果 y 由左子节点，将x设为y的左子节点的父节点
//     if (y->left != T->nil) {
//         y->left->parent = x;
//     }

//     // 第二个方向
//     // 4. 将x的父节点设为y的父节点
//     y->parent = x->parent;
//     // 5. 如果X的父节点为null，则将Y设为根节点；否则，如果X是其父节点的左子节点，则将Y设为X的父节点的左子节点；否则，将Y设为X的父节点的右子节点
//     if (x ->parent == T->nil) {
//         T->root = y;
//     } else if (x == x->parent->left) {
//         x->parent->left = y;
//     } else {
//         x->parent->right = y;
//     }

//     // 第三个方向
//     // 6. 将x设为y的左子节点
//     y->left = x;
//     x->parent = y;
// }

void rbtree_left_rotate(rbtree *T, rbtree_node *x) {

    if (x == T->nil) return ;

    rbtree_node *y = x->right;

    x->right = y->left; // 1
    if (y->left != T->nil) { // 2 如果不是叶子结点，指明其父节点，否则就是个空
        y->left->parent = x;
    }

    y->parent = x->parent; // 3 y->x
    if (x->parent == T->nil) { // 4 如果x的parent = nil，代表x是root结点
        T->root = y;
    } else if (x == x->parent->left) { // 判断是左子树还是右子树
        x->parent->left = y; // 是把y挂载到x的左子树
    } else {
        x->parent->right = y; // 是把y挂载到x的右子树
    }

    y->left = x; // 5
    x->parent = y; // 6
}

void rbtree_right_rotate(rbtree *T, rbtree_node *y) {

    if (y == T->nil) return ;

    rbtree_node *x = y->left;

    y->left = x->right;

    x->parent = y->parent;
    if (y->parent == T->nil) {
        T->root = x;
    } else if (y = y->parent->right) {
        y->parent->right = x;
    } else {
        y->parent->left = x;
    }

    x->right = y;
    y->parent = x;

}

void rbtree_insert_fixup(rbtree *T, rbtree_node *z) {

    /* 
        第一种情况：
            插入375
            父节点是红色的，叔父节点也是红色的，说明之前的高度两者两边对祖父节点而言是垂直的，两边的组织结构 基本上是相同的
            做法：叔父节点和父节点同时变成黑色，祖父节点变成红色
     */

    // z-color == RED
    // z->parent->color == RED
    while (z->parent->color == RED) {

        if (z->parent == z->parent->parent->left) {

            rbtree_node *y = z->parent->parent->right;

            if (y->color == RED) { // 叔父节点是红色的

                z->parent->color = BLACK;
                z->parent->parent->color = RED;
                y->color = BLACK;

                z = z->parent->parent;
            } else { // 叔父节点是黑色
                // 第二种情况：叔父结点是黑色的，而且当前结点是右孩子
                if (z == z->parent->right) {
                    z = z->parent;
                    rbtree_left_rotate(T, z);
                }

                // 第三种情况：叔父结点是黑色的，而且当前结点是左孩子
                z->parent->color = BLACK;
                z->parent->parent->color = RED;
                rbtree_right_rotate(T, z->parent->parent);
            }
        } else {
            rbtree_node *y = z->parent->parent->left;

            if (y->color == RED) {
                z->parent->color = BLACK;
                z->parent->parent->color = RED;
                y->color = BLACK;

                z = z->parent->parent;
            } else {
                if (z == z->parent->right) {
                    z = z->parent;
                    rbtree_right_rotate(T, z);
                }

                z->parent->color = BLACK;
                z->parent->parent->color = RED;
                rbtree_left_rotate(T, z->parent->parent);
            }
        }

    }

    T->root->color = BLACK;

}

// 插入
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

        }

    }

    z->parent = y;
    if (y == T->nil) {
        T->root = z;
    } else if (z->key < y->key) {
        y->left = z;
    } else {
        y->right = z;
    }

    z->color = RED;
    z->left = T->nil;
    z->right = T->nil;



}