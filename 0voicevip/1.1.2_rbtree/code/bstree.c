#include <stdio.h>
#include <stdlib.h> 

#define ARRAY_LENGTH    15

// 二叉树
#if 0
struct bstree_node {
    int data;
    struct bstree_node *left;
    struct bstree_node *right;
};

struct bstree {
    struct bstree_node *root;
};

struct bstree *tree;
#else

typedef int KEY_VALUE;

// 业务和算法相分离
#define BSTREE_ENTRY(name, type)    \
        struct name {               \
            struct type *left;      \
            struct type *right;     \
        }

struct bstree_node {

    // 相当于 struct {} bst;
    BSTREE_ENTRY(, bstree_node) bst;

    KEY_VALUE key;

};

struct bstree {
    struct bstree_node *root;
};

#endif

// 创建节点
struct bstree_node *bstree_create_node(KEY_VALUE key) {

    struct bstree_node *node = (struct bstree_node*)malloc(sizeof(struct bstree_node));
    if (node == NULL) {
        return NULL;
    }

    node->key = key;
    node->bst.left = node->bst.right = NULL;

    return node;
}

// 插入节点
int bstree_insert(struct bstree *tree, int key) {

    if (tree == NULL) return -1;

    // 空树的话直接插入
    if (tree->root == NULL) {
        tree->root = bstree_create_node(key);
        return 0;
    }

    struct bstree_node *node = tree->root;
    struct bstree_node *tmp = tree->root;

    while (node != NULL) {
        tmp = node;

        if (key < node->key) {
            node = node->bst.left;
        } else if (key > node->key) {
            node = node->bst.right;
        } else {
            // ....
            return -1;
        }
    }
    // node = NULL

    if (key < tmp->key) {
        tmp->bst.left = bstree_create_node(key);
    } else {
        tmp->bst.right = bstree_create_node(key);
    }

    return 0;
}

// 遍历
int bstree_traversal(struct bstree_node *node) {

    if (node == NULL) return 0;

    // printf("%4d", node->key); // 前序
    bstree_traversal(node->bst.left);
    printf("%4d", node->key); // 中序
    bstree_traversal(node->bst.right);
    // printf("%4d", node->key); // 后序  

}

int main(int argc, char const *argv[])
{
    int keys[ARRAY_LENGTH] = {25, 67, 89, 90, 15, 78, 56, 69, 16, 26, 47, 10, 29, 61, 63};

    struct bstree tree = {0};

    int i = 0;
    for (i = 0; i < ARRAY_LENGTH; i++) {
        bstree_insert(&tree, keys[i]);
    }

    bstree_traversal(tree.root);
    printf("\n");

    return 0;
}
