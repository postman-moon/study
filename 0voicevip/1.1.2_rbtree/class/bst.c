#include <stdio.h>

typedef int KEY_VALUE;

#if 0
struct bstree_node {
    KEY_VALUE data;
    struct bstree_node *left;
    struct bstree_node *right;
};

struct bstree {
    struct bstree_node *root;
};

struct sched {

    struct bstree_node *ready;
    struct bstree_node *defer;
    struct bstree_node *sleep;
    struct bstree_node *wait;

};

#else

// 业务和数据结构相分离
#define BSTREE_ENTRY(name, type)    \
        struct name {               \
            struct type *left;      \
            struct type *right;     \
        }

struct bstree_node {

    KEY_VALUE data;
    BSTREE_ENTRY(, bstree_node) bst;

};

struct bstree {
    struct bstree_node *root;
};

struct sched {
    BSTREE_ENTRY(, bstree_node) ready;
    BSTREE_ENTRY(, bstree_node) defer;
    BSTREE_ENTRY(, bstree_node) sleep;
    BSTREE_ENTRY(, bstree_node) wait;
};

#endif


struct bstree_node *bstree_create_node(KEY_VALUE key) {

    struct bstree_node *node = (struct bstree_node *)malloc(sizeof(struct bstree_node));
    if (node == NULL) return NULL;

    node->data = key;
    node->bst.left = node->bst.right = NULL;

}

// insert
int bstree_insert(struct bstree *T, KEY_VALUE key) {

    if (T == NULL) return -1;

    if (T->root == NULL) {
        T->root = bstree_create_node(key);
        return 0;
    }

    struct bstree_node *node = T->root;
    struct bstree_node *tmp = T->root;
    while (node != NULL) {
        tmp = node;

        if (key < node->data) {
            node = node->bst.left;
        } else if (key > node->data) {
            node = node->bst.right;
        } else {
            printf("exit!!!\n");
            return -2;
        }
    }
    
    if (key < tmp->data) {
        tmp->bst.left = bstree_create_node(key);
    } else {
        tmp->bst.right = bstree_create_node(key);
    }

    return 0;

}

// traversal
int bstree_traversal(struct bstree_node *node) {

    if (node == NULL) return 0;

    bstree_traversal(node->bst.left);
    printf("%4d", node->data);
    bstree_traversal(node->bst.right);

}